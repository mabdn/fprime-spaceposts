# Transceiver Component Documentation
## Summary
The `Transceiver` component is a passive F' component that receives messages from users on the ground and downlinks messages to users on the ground.
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-TRA-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-TRA-020 | The component shall provide a command for ground station operators to initiate a downlink of a certain number of last stored messages, each as a string | Unit test, integration test
F-TRA-021 | The component shall provide a command for HAM radio users to initiate a downlink of a certain number of last stored messages, each as a string | Unit test, integration test
F-TRA-022 | The component shall provide a way to configure, during runtime, whether the command in F-TRA-021 is executed or rejected when it is received | Unit test
F-TRA-023 | The component shall automatically configure itself to reject commands as in F-TRA-022 every time the satellite reaches critical power | Manual code review
F-TRA-024 | The component shall only execute the command in F-TRA-021 if a certain amount of time has passed since the last downlink of BBS messages by the component, no matter in which way it was triggered | Unit test
F-TRA-025 | The component shall provide a way to configure the amount of time in F-TRA-024 during runtime | Unit test
F-TRA-028 | The component shall provide a scheduling input port which can be connected to the output port of an active rate group. Everytime the port is called, the component initiates a downlink of a certain number of last stored messages as a string | Unit test, Integration test
F-TRA-030 | Downlinking messages shall be performed in one single transmission for each message | Unit test 
F-TRA-050 | In every downlink, the downlinked messages should be the messages which have been stored the most recently measured from the time of the downlink (which could be different from the time of the request) | Integration test

### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-TRA-010 | The number of last messages to downlink in F-TRA-020, F-TRA-021, and F-TRA-028 (same number for both) must be configurable but is constant during runtime | Manual code review
NF-TRA-020 | The component shall be adaptable to scheduling and triggering downlinks from different sources and for different reasons | Manual code review
## Interface to Other Components
## Dependencies
Even though this component requires three other components (see [UserMessage System Component Model](/README.md#component-model)) to provide the functionality that it is supposed to provide in the UserMessage System, the code for this component is completely independent of the components it requires. The components communicate through well-defined F' framework component ports and thus avoid any C++ code dependencies.
  

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

Introduce two separate commands for F-TRA-020 and F-TRA-021 even though they have the same functionality. That simplifies handling during authentication and enabling / disabling command execution.

**Challenge**

Whether the command in F-TRA-021 is executed or rejected must be configurable during runtime. Likewise, the cooldown time in F-TRA-024 must be configurable during runtime.

**Resulting Design Decision**

Provide an F' parameter `ALLOW_HAMUSER_DOWNLINK_CMD` in the component which enables and disables the execution of the command. 

Similarly, the F' parameter `DOWNLINK_COOLDOWN_TIME` configures the number of seconds that must have passed since the last downlink for the HAM radio user downlink command to be executed when received.

Thanks to framework support, the parameters are configurable form the ground station.

**Challenge**

HAM radio user's command for requesting downlinks shall be disabled when the satellite goes into critical power state.

**Resulting Design Decision**

Initialize the configuration parameter for F-TRA-021 to disable the execution. Every time the satellite enter critical power mode, the onboard computer running the flight software is restarted. Thus, the component will be freshly initialized and the execution disabled.

## Test Summary