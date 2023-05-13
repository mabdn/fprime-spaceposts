# MessageTransceiver Component Documentation
## Summary
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-TRA-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-TRA-020 | The component shall provide a command to downlink a certain number of last stored messages | Unit test, integration test
F-TRA-030 | Downlinking messages shall be performed in a separate transmission per message | Unit test 
F-TRA-040 | Downlinking messages shall not be done immediately after the command in F-TRA-020 is received. Instead, downlinks should be schedueled so that the component provides a scheduler input port which triggers downlinking all messages that haven been requested since the last downlink | Unit test 


### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-TRA-010 | The number of last messages to downlink must be configurable but is constant during runtime | Manual code review
## Interface to Other Components