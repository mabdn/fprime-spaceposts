# MessageTransceiver Component Documentation
## Summary
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
F-MOD-010 | The component shall provide a command that stores a given string as a message on the satellite | Unit test, integration test
F-MOD-020 | The component shall provide a command to downlink a certain number of last stored messages | Unit test, integration test
F-MOD-030 | Downlinking messages shall be performed in a separate transmission per message | Unit test 


### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-MOD-010 | The number of last messages to downlink must be configurable but is constant during runtime | Manual code review
## Interface to Other Components