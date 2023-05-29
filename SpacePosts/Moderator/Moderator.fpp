module SpacePosts {

  @ Component with one input port and one output port where `SpacePost`s given to the input port must
  @ pass a moderation check to be output on the output port. 
  @
  @ It has the same interface for storing (i.e., with the same input port type) as the MessageStorage 
  @ component. Thus, it can be optionally plugged in between the Transceiver component and the MessageStorage
  @ component without any of them knowing about the existence of the Moderator component. 
  @ Therefore it is reasonable to have the Moderator as a separate component.
  passive component Moderator {
    
    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Perform a moderation check on the given message and output it on the acceptedMessage port iff it passes the check
    guarded input port moderateMessage: SpacePostSet

    @ Outputs the messages that passed the moderation check
    output port acceptedMessage: SpacePostSet

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Event
    event port eventOut

    @ Telemetry
    telemetry port tlmOut

    @ Text event
    text event port textEventOut

    @ Time get
    time get port timeGetOut

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Downlinking the last SpacePosts stored on the satellite has failed due to an error while downlinking a message
    event MESSAGE_REJECTED \
        severity activity high \
        format "A message failed the moderation check and has thus been rejected" \

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ The number of SpacePosts this component has passed to its output port because they passed the moderation check
    telemetry ACCEPT_COUNT: U32 format "{} uplinks received"

    @ The number of SpacePosts this component has rejected because they did not pass the moderation check
    telemetry REJECT_COUNT: U32 format "{} downlinks performed"
  }
}