module SpacePosts {

  @ Component to receive `UserMessage`s from users on the ground 
  @ and to downlink the `UserMessage`s stored on the satellite to users on the ground.
  @
  @ It is a separate component because it encapsulates how users can trigger the loading
  @ and storing of `UserMessage`s on the satellite. Consequently, the Transceiver 
  @ implementation can be swapped out to change how the satellite communicates messages with users on the ground.
  passive component Transceiver {
    
    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Store a single given message at the next available index
    guarded input port scheduleDownlink: Svc.Sched

    @ Store a single message in the satellite's storage 
    output port storeMessage: SpacePostSet

    @ Load a certain number N of messages from the satellite's storage
    output port loadMessages: SpacePostGetLastN

    @ Downlink a single message by passing it to this output port
    output port downlinkMessage: Fw.Com


    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Command receive port
    command recv port cmdIn

    @ Command registration port
    command reg port cmdRegOut

    @ Command response port
    command resp port cmdResponseOut

    @ Event
    event port eventOut

    @ Telemetry
    telemetry port tlmOut

    @ Parameter get
    param get port prmGetOut

    @ Parameter set
    param set port prmSetOut

    @ Text event
    text event port textEventOut

    @ Time get
    time get port timeGetOut

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Store the given SpacePosts on the satellite
    guarded command STORE_MESSAGE(
        @ The message to store on the satellite
        msg: SpacePost
    )

    @ Initiate the downlink of the last SpacePosts stored on the satellite by a ground station operator 
    @
    @ See design requirement F-TRA-020
    guarded command DOWNLINK_LAST_MESSAGES_GDS

    @ Initiate the downlink of the last SpacePosts stored on the satellite by a HAM radio user 
    @
    @ Separate from DOWNLINK_LAST_MESSAGES_GDS because the HAM radio user access to this command may be restricted
    @ by:
    @ * ALLOW_HAMUSER_DOWNLINK_CMD parameter: Ground station operators can disable the DOWNLINK_LAST_MESSAGES_HAMUSER 
    @   command in a way that it does not trigger a downlink.
    @ * DOWNLINK_COOLDOWN_TIME parameter: Ground station operators can set a cooldown time after which must have passed
    @   since the last downlink. Otherwise, the HAM radio user downlink command does not trigger a downlink. 
    @
    @ See design requirement F-TRA-021
    guarded command DOWNLINK_LAST_MESSAGES_HAMUSER


    # ----------------------------------------------------------------------
    # Parameters
    # ----------------------------------------------------------------------

    @ Enables and disables the DOWNLINK_LAST_MESSAGES_HAMUSER command.
    @
    @ See design requirements F-TRA-022 and F-TRA-023
    param ALLOW_HAMUSER_DOWNLINK_CMD: bool default false

    @ The number of seconds that must have passed since the last downlink for the HAM radio user downlink command 
    @ to be executed when received.
    @
    @ I.e., if DOWNLINK_LAST_MESSAGES_HAMUSER is received and the last downlink was performed less than
    @ DOWNLINK_COOLDOWN_TIME seconds ago, no downlink is performed.
    param DOWNLINK_COOLDOWN_TIME: U32 default 3600

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Downlinking the last SpacePosts stored on the satellite has failed due to an error while downlinking a message
    event DOWNLINK_FAILED(
        index: U32 @< The index of the message that failed to downlink
        # TODO: Add parameters or remove event. Depends on how throwing errors while downlinking is implemented
        # error: TRANSCEIVER_DOWNLINK_ERROR @< Error Code that provides more information about why the downlink failed
        ) \
        severity warning high \
        format "Downlinking message with index #{} failed" \

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ The number of SpacePosts received for storage on the satellite
    telemetry UPLINK_COUNT: U32 format "{} uplinks received"

    @ The number of times this component has initiated a downlink of the last SpacePosts stored on the satellite
    telemetry DOWNLINK_COUNT: U32 format "{} downlinks performed"
  }
}