module SpacePosts {

  @ Component for receiving and storing messages which have a string as their primary content.
  @
  @ For the convenience of the user, this component specifically stores data of type SpacePost. 
  @ However, internally, I should abstract from the members
  @ of this type as much as possible. I.e., the component knows that it stores SpacePosts but does not know what members
  @ a SpacePost has. The only assumption this component really needs to make is that a SpacePost is a child of 
  @ Serializable. 
  @ In other words, for storing on the satellite, a SpacePost does not need to be unpacked from its format.
  @
  @ Thus, this component needs no to minimal change if the definition of the SpacePost type changes.
  @ Furthermore, as this component is build to generally store any Serializable, it can easily be re-used / refactored
  @ for storing other data (e.g. MEMEs).
  passive component MessageStorage {

    # ----------------------------------------------------------------------
    # Types
    # ----------------------------------------------------------------------

    @ Stages of writing a SpacePost to the file system in which an error can occur
    enum MessageWriteError {
      FILE_EXISTS @< A .spacepost file with the specified index already exists
      OPEN @< File OSAL call to open the file failed
      DELIMITER_WRITE @< Writing the delimiter to the file failed
      DELIMITER_SIZE @< Writing the delimiter to the file did not write the expected number of bytes
      MESSAGE_SIZE_WRITE @< Writing the message size to the file failed
      MESSAGE_SIZE_SIZE @< Writing the message size to the file did not write the expected number of bytes
      MESSAGE_CONTENT_WRITE @< Writing the message content to the file failed
      MESSAGE_CONTENT_SIZE @< Writing the message content to the file did not write the expected number of bytes
      CLEANUP_DELETE @< Deleting the file after an error occurred failed
    }

    @ Stages of reading a SpacePost from the file system in which an error can occur
    enum MessageReadError {
      OPEN @< File OSAL call to open the file for the specified index failed. Maybe the file does not exist
      DELIMITER_READ @< Reading the delimiter from the file failed
      DELIMITER_SIZE @< Reading the delimiter from the file did not read the expected number of bytes
      DELIMITER_CONTENT @< The delimiter read from the file does not match the expected delimiter
      MESSAGE_SIZE_READ @< Reading the message size from the file failed
      MESSAGE_SIZE_SIZE @< Reading the message size from the file did not read the expected number of bytes
      MESSAGE_SIZE_DESER_SET_LENGTH @< Setting the length of the deserialization buffer for deserializing the message size failed
      MESSAGE_SIZE_DESER_EXCECUTE @< Deserializing the message size failed. 
      MESSAGE_SIZE_DESER_READ_LENGTH @< Deserializing the message size did not use the expected number of bytes
      MESSAGE_SIZE_EXCEEDS_BUFFER @< The message size read from the file exceeds the size of the deserialization buffer.
      MESSAGE_SIZE_ZERO @< The message size read from the file is zero.
                        @< This is invalid as the component would have never stored a message with a size of zero.
      MESSAGE_CONTENT_READ @< Reading the message content from the file failed
      MESSAGE_CONTENT_SIZE @< Reading the message content from the file did not read the expected number of bytes
      MESSAGE_CONTENT_DESER_SET_LENGTH @< Setting the length of the deserialization buffer for deserializing the message content failed
      MESSAGE_CONTENT_DESER_EXCECUTE @< Deserializing the message content failed.
                                     @< 
                                     @< One of the reasons for this error is that
                                     @< the message fits into the deserialization buffer but is malformed 
                                     @< (e.g., missing a null terminator for strings).
      MESSAGE_CONTENT_DESER_READ_LENGTH @< Deserializing the message content did not use the expected number of bytes
      FILE_END @< Parsing the message from the file ended before the end of the file was reached. 
               @< I.e., the file contained more data than expected
    }


    @ Stages of restoring the index from the storage directory in which an error can occur
    enum IndexRestoreError {
      STORAGE_DIR_OPEN @< Opening the storage directory failed
      STORAGE_DIR_READ @< Reading the file names from the storage directory ended with an error instead of 
                       @< OS::Directory::NO_MORE_FILES
    }



    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Store a single given message at the next available index
    guarded input port storeMessage: SpacePostSet

    @ Load a single stored message by index
    guarded input port loadMessageFromIndex: SpacePostGetFromIndex

    @ Load the first n messages which have been stored the most recently and can be successfully loaded
    @ 
    @ The returned messages are ordered in inverse chronological order of storing.
    @ I.e., the first message in the returned batch is the most recently stored message.
    @
    @ If loading a message within the last n stored messages fails, the component will 
    @ attempt to load the next most recently stored message and add it to the batch.
    @ Consequently, the returned batch contains n or less messages which were successfully loaded
    @ (also see definition of SpacePostGetLastN).
    guarded input port loadMessageLastN: SpacePostGetLastN

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

    @ A SpacePost has been successfully written to the file system
    event MESSAGE_STORE_COMPLETE(
                          storage_index: U32 @< The index at which the message has been stored
                        ) \
      severity activity low \
      format "Message stored at index {d}" 

    @ An unhandled error occurred while attempting to write a SpacePost to the file system
    event MESSAGE_STORE_FAILED(
                                  storage_index: U32 @< The index at which the message was supposed to be stored
                                  stage: MessageWriteError @< The stage of storing the message in which 
                                                           @< the error occurred
                                  error_code: I32 @< Additional error code of the specified stage. 
                                                  @< Meaning depends on stage
                             ) \
      severity warning high \
      format "Failed to store message #{} in stage {} with error {}" 
    
    @ A SpacePost has been successfully read from the file system
    event MESSAGE_LOAD_COMPLETE(
                                storage_index: U32 @< The index from which the message has been loaded
                              ) \
      severity activity low \
      format "Message loaded from index {d}"

    @ An unhandled error occurred while attempting to read a SpacePost from the file system
    event MESSAGE_LOAD_FAILED(
                                storage_index: U32 @< The index from which the message was supposed to be loaded
                                stage: MessageReadError @< The stage of loading the message in which 
                                                        @< the error occurred
                                error_code: I32 @< Additional error code of the specified stage. 
                                                  @< Meaning depends on stage
                             ) \
      severity warning low \
      format "Failed to load message #{} in stage {} with error {}"

    @ Index was restored from the storage directory by finding the highest index in use. 
    @ 
    @ The component sets the index to 0 if no SpacePost was found in the storage directoy.
    @
    @ Emitted upon intialization of the component.
    event INDEX_RESTORE_COMPLETE(
                                  num_messages_found: U32 @< The number of messages found in the storage directory
                                  index: U32 @< The highest index found in use by a stored message in the storage directory
                                             @< 0 if no messages were found in the storage directory
                                ) \
      severity activity low \
      format "Found highest index i={} among {} files in the storage directory. Next message will be stored with index i+1" \

    @ An unhandled error occurred while attempting to restore the index from the storage directory
    @
    @ The component's index was not changed based on what was found in the storage directory
    event INDEX_RESTORE_FAILED(
                                stage: IndexRestoreError @< The stage of restoring the index in which the error occurred
                                error_code: I32 @< Additional error code of the specified stage
                              ) \
      severity warning high \
      format "Failed to restore index from storage directory in stage {} with error {}" \

    @ The index of SpacePosts reached the maximum value of U32 and was thus reset to 0.
    event INDEX_WRAP_AROUND \
      severity warning low \
      format "Next SpacePost index wrapped around from MAX_U32 to 0" \


    @ The component was not able to open the specified storage directory. E.g., because it does not exist
    event STORAGE_DIRECTORY_WARNING(
                                    directory: string size 128  @< The absolute path of the storage directory which 
                                                                @< the MessageStorage component tried to use for 
                                                                @< storing messages
                                    was_able_to_create: bool @< True, iff the component created the directory and should
                                                             @< proceed without errors.
                                                             @< False, iff the component was not able to create the 
                                                             @< directory. Thus, an error is highly likely upon the 
                                                             @< subsequent write.
                                   ) \
      severity warning low \
      format "Storage directory '{}' did not exist. Was component able to fix the error by creating the directoy?: {}." 

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ The index at which the next message will be stored
    @
    @ Emitted upon intialization of the component and after each attempt to store a message / 
    @ call to the storeMessage port. 
    telemetry NEXT_STORAGE_INDEX: U32 id 1 \
      format "Next message will be stored at index {}"

    @ The number of messages that have been attempted to be stored since the component was started
    telemetry STORE_COUNT: U32 id 2 \
      format "Number of messages stored: {}"

    @ The number of messages that have been attempted to be loaded since the component was started
    telemetry LOAD_COUNT: U32 id 3 \ 
      format "Number of messages loaded: {}"
  }

}