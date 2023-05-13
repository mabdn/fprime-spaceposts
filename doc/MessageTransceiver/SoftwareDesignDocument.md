# MessageTransceiver Component Documentation
## Summary
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-TRA-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-TRA-020 | The component shall provide a command to request a downlink of a certain number of last stored messages as a string | Unit test, integration test
F-TRA-030 | Downlinking messages shall be performed in a separate transmission per message | Unit test 
F-TRA-040 | Downlinking messages shall not be done immediately after the command in F-TRA-020 is received. Instead, the component shall provide a scheduling input port which triggers a downlink of the requested messages if there has been a request since the last call of the scheduling port | Unit test 
F-TRA-050 | In every downlink, the downlinked messages should be the messages which have been stored the most recently measured from the time of the downlink (which can be different from the time of the request) | Integration test


### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-TRA-010 | The number of last messages to downlink in F-TRA-020 must be configurable but is constant during runtime | Manual code review
## Interface to Other Components
## Dependencies
## Internal Design
### Downlinking Messages
**Challenge**

Multiple messages need to be downlinked in response to the ground station in response to receiving a F' command.

**Resulting Design Decision**

The mes


## Test Summary