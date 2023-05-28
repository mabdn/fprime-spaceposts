module SpacePosts {

  @ The maximum number of characters in a SpacePost's message content text field 
  @ including a compulsory null-terminator at the end of the string.
  @
  @ I.e. this is the maximum length of a char buffer that needs to hold a SpacePost's text content. 
  @
  @ When creating a SpacePost from a c string, this maximum length and placing the null-terminator is automatically
  @ enforced. I.e., the string is truncated if necessary.
  constant SpacePost_MaxCStrLength = 256 

  @ The maximum number of characters in a SpacePost's message content text field 
  @ excluding the compulsory null-terminator at the end of the string.
  @
  @ I.e. this is the maximum number of characters that can be used for text content in a SpacePost.
  constant SpacePost_MaxTextLength = SpacePost_MaxCStrLength - 1 

  @ The maximum number of messages that can be stored in a SpacePost_Batch.
  @ This is a constant because it is used to define the size of the array of SpacePosts in a SpacePost_Batch.
  @
  @ Components throughout the SpacePost system can (and currently do) refer to this as the maximum number of messages 
  @ that can be / are loaded and downlinked at once.
  constant SpacePost_Batch_Size = 30
  
  @ An array of SpacePosts to be used in a SpacePost_Batch. Not supposed to be used outside / without a 
  @ SpacePost_Batch.
  array SpacePost_Array = [SpacePost_Batch_Size] SpacePost

  @ A set of multiple SpacePosts that are sent together. 
  @
  @ For instance, this datatype can be used to load or pass multiple messages around the satellite at once.
  @ Note that is not suitable for use outside of the satellite becasuse SpacePosts are currently only implemented
  @ as pointers to dynamic heap memory where the actual message's content is stored.
  @
  @ A batch has a fixed maximum capacity of messages (SpacePost_Batch_Size).
  @ The number of valid messages in the batch is stored in 'numValidMessages'. It is always smaller equals
  @ SpacePost_Batch_Size.
  @ The first 'numValidMessages' entries in the 'messages' array are valid and contain the messages present in 
  @ this batch. 
  struct SpacePost_Batch {
    @ The number of SpacePosts, each of which is one entry in the messages array, that are
    @ present in this batch.
    numValidMessages: U8 \
      format "{} messages" 

    messages: SpacePost_Array @< The messages in this batch. The first 'numValidMessages' entries are valid.

  } default { numValidMessages = 0 }

  @ The data type in which a SpacePost is handled for storing it on the satellite.
  @
  @ Note that the representation of the message on the satellite (i.e. just a fixed-size string of length 256) can be 
  @ very different from a HAM radio-user's or the ground station's perspective (e.g. in a JSON file format). 
  struct SpacePost {
      @ The SpacePost's text content.
      @
      @ When creating a SpacePost from a c string, its maximum length and placing a null-terminator at the end
      @ is automatically enforced. I.e., the string is truncated if necessary. 
      @ The maximum number of characters (excl. the null-terminator) for the message_content is thus 
      @ SpacePost_MaxCStrLength - 1.
      @
      @ The message_content may contain any byte values as characters expect for the null-terminator (0x00).
      @ The null-terminator is reserved for marking the end of message_content string in a char buffer.
      message_content: string size SpacePost_MaxCStrLength 
  }

}