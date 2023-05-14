# Moderator Component Documentation
## Summary
The `Moderator` component is a passive F' component that decides whether received messages are stored or discared based on a set of moderation rules.
## Requirements
### Functional Requirements
Requirement | Description | Verification Method
---- | ---- | --------------
F-MOD-010 | The component shall provide an input port that accepts a message | Unit Test
F-MOD-020 | The component shall provide an output port that outputs every message received on the input port in F-MOD-010 iff the message passes a moderation check | Unit Test
F-MOD-030 | The component's moderation check shall let a message pass the moderation check in F-MOD-020 iff it meets all of the specified moderation criteria M-MOD-* | Unit Test


### Moderation Criteria
Criteria | Description | 
----------- | ---------------------- | 
M-MOD-010 | A message only contains letter

### Non-functional Requirements
Requirement | Description | Verification Method
----------- | ----------- | -------------------
NF-MOD-010 | The component shall be highly adaptable and extendible to different and new moderation criteria | Manual code review

## Necessity
TODO: Why is this component needed? (I.e., why cannt we moderate on the ground)
TODO: Is this component needed?

## Interface to Other Components