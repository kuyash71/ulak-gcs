# SCENARIO 1: DRAWING THE INFINITY AROUND TWO POLES

_INFO_: Scenario for TEKNOFEST's first mission for rotary-winged drone category
_ALGORITHM_: This Scenario's purpose is finishing the first mission with Image Processing Algorithm
_VERSION_: 0.1
_STATUS_: The scenario is only on test stage. Not Finished

---

## STAGE 1: PREPERATION

This section wont change between scenarios. The purpose of this section is checking and confirming the
status of the drone and taking the first takeoff.

- CONNECT: Drone
- SET: Mode: Stabilize
- CHECK: General healthcheck (Especially IMU and GPS)
- SET: Mode: Guided
- ARM: Throttle
- Takeoff: 10

## STAGE 2: DETECTING POLE

- OPENCV: Read Camera data and Look for Poles
- CHECK: The amount of poles on screen.
- LOOP:
  While the amount of poles is not 2:
  While the amount of poles is 1: Search for second pole by using the only poles location (is it is on left we can think that the other one will be on its west)
  While the amount of poles is 0: Search for poles by turning clock-wise
  Else: Turn ERROR on terminal: The OpenCV method is not healty for this simulation. RTL for simulation

### STAGE 3: AIMING FOR POLE

-SET: Set the TARGET_POLE as the Pole on right. It will be labeled as "TARGET" for now

- LOOP:
  Change the direction of drone for aiming the drone towards to TARGET
  Fly to TARGET while there is a second pole on camera data

SCENARIO ENDS.
