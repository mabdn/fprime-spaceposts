# Transceiver Component Documentation
## Summary
The `Transceiver` component is a passive F' component that receives messages from users on the ground and downlinks messages to users on the ground.
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-TRA-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-TRA-020 | The component shall provide a command for ground station operators to initiate a downlink of a certain number of last stored messages as a string | Unit test, integration test
F-TRA-021 | The component shall provide a command for HAM radio users to initiate a downlink of a certain number of last stored messages as a string | Unit test, integration test
F-TRA-022 | The component shall provide a way to configure during runtime whether the command in F-TRA-021 is executed or rejected | Unit test
F-TRA-023 | The component shall automatically configure itself to reject commands as in F-TRA-022 every time the satellite reaches critical power | Manual code review
F-TRA-025 | The component shall provide a scheduling input port which can be connected to the ouput port of an active rate group. Everytime the port is called, the component initiates a downlink of a certain number of last stored messages as a string | Unit test, Integration test
F-TRA-030 | Downlinking messages shall be performed in a separate transmission per message | Unit test 
F-TRA-050 | In every downlink, the downlinked messages should be the messages which have been stored the most recently measured from the time of the downlink (which can be different from the time of the request) | Integration test


### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-TRA-010 | The number of last messages to downlink in F-TRA-020, F-TRA-021, and F-TRA-025 (same number for both) must be configurable but is constant during runtime | Manual code review
NF-TRA-020 | The component shall be adaptable to scheduling and triggering downlinks from different sources and for different reasons | Manual code review
## Interface to Other Components
## Dependencies
* Framer for downlinking
* CommandScheduler for receiving commands from ground station
* Moderator and MessageStorage
## Internal Design
### Downlinking Messages

**Challenge**

Multiple messages need to be downlinked to the ground station.

**Resulting Design Decision**

The messages are transmitted in multiple transmissions with one transmission per message. Thus, the packet size is kept small. Furthermore, if a transmission fails, some messages are potentially successfully transmitted while only some others fail.


### Requesting Downlinks
**Challenge** 

Access to the command for requesting a downlink of messages shall always be available to ground station operators but restrictable for HAM radio users.

**Resulting Design Decision**

Introduce two separate commands for F-TRA-020 and F-TRA-021 even though the have the same functionality. That simplifies handling during authentication and enabling / disabling command execution.

**Challenge**

Whether the command in F-TRA-021 is executed or rejected must be configurable during runtime.

**Resulting Design Decision**

Provide an F' parameter in the component which enables and disables the execution of the command. Thanks to framework support, the parameter is configurable form the ground station.

**Challenge**

HAM radio user's command for requesting downlinks shall be disabled when the satellite goes into critical power state.

**Resulting Design Decision**

Initialize the configuration parameter for F-TRA-021 to disable the execution. Every time the satellite enter critical power mode, the onboard computer running the flight software is restarted. Thus, the component will be freshly initialized and the execution disabled.

## Test Summary