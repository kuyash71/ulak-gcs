#!/usr/bin/env python3

from __future__ import annotations

import math
import re
import signal
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from pymavlink import mavutil


FLOAT_RE = r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?"
POSITION_VELOCITY_MASK = (
    mavutil.mavlink.POSITION_TARGET_TYPEMASK_AX_IGNORE
    | mavutil.mavlink.POSITION_TARGET_TYPEMASK_AY_IGNORE
    | mavutil.mavlink.POSITION_TARGET_TYPEMASK_AZ_IGNORE
    | mavutil.mavlink.POSITION_TARGET_TYPEMASK_YAW_IGNORE
    | mavutil.mavlink.POSITION_TARGET_TYPEMASK_YAW_RATE_IGNORE
)

# User-editable parameters (4)
MAVLINK_CONNECTION = "udp:127.0.0.1:14550"
FLIGHT_ALTITUDE_M = 12.0
TRAJECTORY_SPEED_MPS = 10.0
SETPOINT_RATE_HZ = 20.0

# Internal defaults
POLE_FILE = Path(__file__).with_name("pole_data.txt")
BAUDRATE = 57600
RUN_DURATION_S = 0.0
START_HOLD_S = 5.0
HEARTBEAT_TIMEOUT_S = 30.0
TAKEOFF_TIMEOUT_S = 60.0
EXIT_MODE = "LOITER"  # NONE | LOITER | RTL | LAND
SKIP_ARM = False
SKIP_TAKEOFF = False


@dataclass(frozen=True)
class GazeboPose:
    x: float
    y: float
    z: float


@dataclass(frozen=True)
class LocalNED:
    north: float
    east: float
    down: float


@dataclass(frozen=True)
class TrajectorySample:
    north: float
    east: float
    down: float
    vn: float
    ve: float
    vd: float


class InfinityTrajectory:
    """
    Build an infinity path with two circular loops around two centers.
    Loop-1: center-1 clockwise
    Loop-2: center-2 counter-clockwise
    """

    def __init__(self, center_1: LocalNED, center_2: LocalNED, speed_mps: float):
        if speed_mps <= 0:
            raise ValueError("speed_mps must be > 0")

        dx = center_2.north - center_1.north
        dy = center_2.east - center_1.east
        distance = math.hypot(dx, dy)
        if distance <= 1e-6:
            raise ValueError("Pole centers are too close to build an infinity path")

        self.center_1 = center_1
        self.center_2 = center_2
        self.speed_mps = speed_mps
        self.radius = distance * 0.5
        self.u_n = dx / distance
        self.u_e = dy / distance
        self.v_n = -self.u_e
        self.v_e = self.u_n
        self.loop_time = (2.0 * math.pi * self.radius) / self.speed_mps
        self.period = 2.0 * self.loop_time
        self.omega = self.speed_mps / self.radius
        self.flight_down = center_1.down

    def sample(self, elapsed_s: float) -> TrajectorySample:
        phase = elapsed_s % self.period
        if phase < self.loop_time:
            theta = -self.omega * phase
            center = self.center_1
            c = math.cos(theta)
            s = math.sin(theta)
            north = center.north + self.radius * (c * self.u_n + s * self.v_n)
            east = center.east + self.radius * (c * self.u_e + s * self.v_e)
            vn = self.speed_mps * (s * self.u_n - c * self.v_n)
            ve = self.speed_mps * (s * self.u_e - c * self.v_e)
        else:
            tau = phase - self.loop_time
            theta = math.pi + self.omega * tau
            center = self.center_2
            c = math.cos(theta)
            s = math.sin(theta)
            north = center.north + self.radius * (c * self.u_n + s * self.v_n)
            east = center.east + self.radius * (c * self.u_e + s * self.v_e)
            vn = self.speed_mps * (-s * self.u_n + c * self.v_n)
            ve = self.speed_mps * (-s * self.u_e + c * self.v_e)

        return TrajectorySample(
            north=north,
            east=east,
            down=self.flight_down,
            vn=vn,
            ve=ve,
            vd=0.0,
        )


def load_poles_from_file(path: Path) -> tuple[GazeboPose, GazeboPose]:
    text = path.read_text(encoding="utf-8")
    conversion_patterns = (
        r"Ardupilot_X\s*=\s*Gazebo_Y",
        r"Ardupilot_Y\s*=\s*Gazebo_X",
        r"Ardupilot_Z\s*=\s*-\s*Gazebo\s*Z",
    )
    if not all(re.search(pattern, text, flags=re.IGNORECASE) for pattern in conversion_patterns):
        print(
            "Warning: conversion rule block not found exactly. "
            "Using Ardupilot_X=Gazebo_Y, Ardupilot_Y=Gazebo_X, Ardupilot_Z=-Gazebo_Z.",
            file=sys.stderr,
        )

    axis_pairs = []
    for match in re.finditer(rf"^\s*([XYZxyz])\s*:\s*({FLOAT_RE})\s*$", text, flags=re.MULTILINE):
        axis_pairs.append((match.group(1).upper(), float(match.group(2))))

    if len(axis_pairs) < 6:
        raise ValueError(f"Expected at least 6 axis values in {path}, found {len(axis_pairs)}")

    poles: list[GazeboPose] = []
    idx = 0
    for _ in range(2):
        expected = {}
        for axis in ("X", "Y", "Z"):
            if idx >= len(axis_pairs):
                raise ValueError(f"Missing {axis} value while parsing {path}")
            found_axis, value = axis_pairs[idx]
            if found_axis != axis:
                raise ValueError(
                    f"Unexpected axis order in {path}: expected {axis}, found {found_axis}"
                )
            expected[axis] = value
            idx += 1
        poles.append(GazeboPose(x=expected["X"], y=expected["Y"], z=expected["Z"]))

    return poles[0], poles[1]


def gazebo_to_ardupilot_local_ned(gazebo_pose: GazeboPose) -> LocalNED:
    return LocalNED(
        north=gazebo_pose.y,
        east=gazebo_pose.x,
        down=-gazebo_pose.z,
    )


def connect_vehicle(connection: str, baud: int, heartbeat_timeout_s: float):
    master = mavutil.mavlink_connection(connection, baud=baud)
    hb = master.wait_heartbeat(timeout=heartbeat_timeout_s)
    if hb is None:
        raise TimeoutError(f"No heartbeat on {connection} in {heartbeat_timeout_s:.1f}s")

    print(
        f"Connected: sys={master.target_system} comp={master.target_component} "
        f"mode={mavutil.mode_string_v10(hb)}"
    )
    return master


def set_mode(master, mode_name: str, timeout_s: float = 10.0):
    mode_map = master.mode_mapping()
    if not mode_map:
        raise RuntimeError("Mode mapping not available from vehicle")

    selected_name = None
    for key in mode_map:
        if key.upper() == mode_name.upper():
            selected_name = key
            break
    if selected_name is None:
        raise RuntimeError(f"Requested mode '{mode_name}' not found. Available: {sorted(mode_map)}")

    mode_id = mode_map[selected_name]
    master.mav.set_mode_send(
        master.target_system,
        mavutil.mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
        mode_id,
    )

    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        hb = master.recv_match(type="HEARTBEAT", blocking=True, timeout=1.0)
        if hb and hb.custom_mode == mode_id:
            print(f"Mode set: {selected_name}")
            return

    raise TimeoutError(f"Mode change to {selected_name} timed out")


def arm_vehicle(master, timeout_s: float = 15.0):
    master.arducopter_arm()
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        master.recv_match(type="HEARTBEAT", blocking=True, timeout=1.0)
        if master.motors_armed():
            print("Vehicle armed")
            return
    raise TimeoutError("Arming timed out")


def current_relative_alt_m(master, timeout_s: float = 1.0) -> float | None:
    msg = master.recv_match(
        type=["LOCAL_POSITION_NED", "GLOBAL_POSITION_INT"],
        blocking=True,
        timeout=timeout_s,
    )
    if msg is None:
        return None
    if msg.get_type() == "LOCAL_POSITION_NED":
        return -float(msg.z)
    return float(msg.relative_alt) / 1000.0


def takeoff_to_altitude(master, altitude_m: float, timeout_s: float = 45.0):
    if altitude_m <= 0:
        raise ValueError("Takeoff altitude must be > 0")

    master.mav.command_long_send(
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_CMD_NAV_TAKEOFF,
        0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        altitude_m,
    )
    print(f"Takeoff requested: {altitude_m:.1f} m")

    target = altitude_m * 0.92
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        alt = current_relative_alt_m(master, timeout_s=1.0)
        if alt is not None:
            print(f"Altitude: {alt:.2f} m", end="\r", flush=True)
            if alt >= target:
                print(f"\nTakeoff reached: {alt:.2f} m")
                return

    raise TimeoutError("Takeoff altitude not reached in time")


def send_local_setpoint(master, sample: TrajectorySample):
    boot_ms = int(time.time() * 1000) & 0xFFFFFFFF
    master.mav.set_position_target_local_ned_send(
        boot_ms,
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_FRAME_LOCAL_NED,
        POSITION_VELOCITY_MASK,
        sample.north,
        sample.east,
        sample.down,
        sample.vn,
        sample.ve,
        sample.vd,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
    )


def hold_position(master, north: float, east: float, down: float, seconds: float, rate_hz: float):
    if seconds <= 0:
        return

    static_sp = TrajectorySample(
        north=north,
        east=east,
        down=down,
        vn=0.0,
        ve=0.0,
        vd=0.0,
    )
    dt = 1.0 / rate_hz
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        send_local_setpoint(master, static_sp)
        time.sleep(dt)


def main() -> int:
    if SETPOINT_RATE_HZ <= 0:
        print("SETPOINT_RATE_HZ must be > 0", file=sys.stderr)
        return 2
    if TRAJECTORY_SPEED_MPS <= 0:
        print("TRAJECTORY_SPEED_MPS must be > 0", file=sys.stderr)
        return 2
    if FLIGHT_ALTITUDE_M <= 0:
        print("FLIGHT_ALTITUDE_M must be > 0", file=sys.stderr)
        return 2
    if EXIT_MODE not in {"NONE", "LOITER", "RTL", "LAND"}:
        print("EXIT_MODE must be one of: NONE, LOITER, RTL, LAND", file=sys.stderr)
        return 2
    if not POLE_FILE.exists():
        print(f"Pole file not found: {POLE_FILE}", file=sys.stderr)
        return 2

    pole_1_gz, pole_2_gz = load_poles_from_file(POLE_FILE)
    pole_1_ap = gazebo_to_ardupilot_local_ned(pole_1_gz)
    pole_2_ap = gazebo_to_ardupilot_local_ned(pole_2_gz)

    flight_down = ((pole_1_ap.down + pole_2_ap.down) * 0.5) - FLIGHT_ALTITUDE_M
    center_1 = LocalNED(north=pole_1_ap.north, east=pole_1_ap.east, down=flight_down)
    center_2 = LocalNED(north=pole_2_ap.north, east=pole_2_ap.east, down=flight_down)
    trajectory = InfinityTrajectory(
        center_1=center_1,
        center_2=center_2,
        speed_mps=TRAJECTORY_SPEED_MPS,
    )
    cross = trajectory.sample(0.0)

    print("Loaded poles (Gazebo):")
    print(f"  Pole1: x={pole_1_gz.x:.2f}, y={pole_1_gz.y:.2f}, z={pole_1_gz.z:.2f}")
    print(f"  Pole2: x={pole_2_gz.x:.2f}, y={pole_2_gz.y:.2f}, z={pole_2_gz.z:.2f}")
    print("Converted poles (ArduPilot local NED):")
    print(f"  Pole1: N={pole_1_ap.north:.2f}, E={pole_1_ap.east:.2f}, D={pole_1_ap.down:.2f}")
    print(f"  Pole2: N={pole_2_ap.north:.2f}, E={pole_2_ap.east:.2f}, D={pole_2_ap.down:.2f}")
    print(
        f"Infinity radius={trajectory.radius:.2f}m speed={trajectory.speed_mps:.2f}m/s "
        f"period={trajectory.period:.2f}s altitude={FLIGHT_ALTITUDE_M:.2f}m"
    )
    print(
        f"Crossing point: N={cross.north:.2f}, E={cross.east:.2f}, D={cross.down:.2f} "
        f"(hold {START_HOLD_S:.1f}s)"
    )

    stop_requested = {"value": False}

    def _handle_stop(_signum, _frame):
        stop_requested["value"] = True

    signal.signal(signal.SIGINT, _handle_stop)
    signal.signal(signal.SIGTERM, _handle_stop)

    master = connect_vehicle(MAVLINK_CONNECTION, BAUDRATE, HEARTBEAT_TIMEOUT_S)
    try:
        set_mode(master, "GUIDED")
        if not SKIP_ARM:
            arm_vehicle(master)
        else:
            print("Skipping arm as requested")

        if not SKIP_TAKEOFF:
            takeoff_to_altitude(master, FLIGHT_ALTITUDE_M, timeout_s=TAKEOFF_TIMEOUT_S)
        else:
            print("Skipping takeoff as requested")

        hold_position(
            master=master,
            north=cross.north,
            east=cross.east,
            down=cross.down,
            seconds=START_HOLD_S,
            rate_hz=SETPOINT_RATE_HZ,
        )

        print("Starting infinity trajectory. Press Ctrl+C to stop.")
        start_time = time.monotonic()
        dt = 1.0 / SETPOINT_RATE_HZ
        next_tick = start_time
        last_phase = 0.0
        cycle_count = 0

        while not stop_requested["value"]:
            elapsed = time.monotonic() - start_time
            if RUN_DURATION_S > 0 and elapsed >= RUN_DURATION_S:
                break

            sample = trajectory.sample(elapsed)
            send_local_setpoint(master, sample)

            phase = elapsed % trajectory.period
            if phase < last_phase:
                cycle_count += 1
                print(f"Completed infinity cycle: {cycle_count}")
            last_phase = phase

            next_tick += dt
            sleep_time = next_tick - time.monotonic()
            if sleep_time > 0:
                time.sleep(sleep_time)
            else:
                next_tick = time.monotonic()

    finally:
        if EXIT_MODE != "NONE":
            try:
                set_mode(master, EXIT_MODE)
            except Exception as exc:  # noqa: BLE001
                print(f"Failed to set exit mode {EXIT_MODE}: {exc}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
