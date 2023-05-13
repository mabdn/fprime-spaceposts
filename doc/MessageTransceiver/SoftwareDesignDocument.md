# MessageTransceiver Component Documentation
## Summary
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-TRA-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-TRA-020 | The component shall provide a command to initiate a downlink of a certain number of last stored messages as a string | Unit test, integration test
F-TRA-025 | The component shall provide a scheduling input port which can be connected to the ouput port of an active rate group. Everytime the port is called, the component initiates a downlink of a certain number of last stored messages as a string | Unit test, Integration test
F-TRA-030 | Downlinking messages shall be performed in a separate transmission per message | Unit test 
F-TRA-050 | In every downlink, the downlinked messages should be the messages which have been stored the most recently measured from the time of the downlink (which can be different from the time of the request) | Integration test


### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-TRA-010 | The number of last messages to downlink in F-TRA-020 and F-TRA-025 (same number for both) must be configurable but is constant during runtime | Manual code review
NF-TRA-020 | The component shall be adaptable to scheduling and triggering downlinks from different sources and for different reasons | Manual code review
## Interface to Other Components
## Dependencies
## Internal Design
### Downlinking Messages
**Challenge**

Multiple messages need to be downlinked in response to the ground station in response to receiving a F' command.

**Resulting Design Decision**

The mes


## Test Summary