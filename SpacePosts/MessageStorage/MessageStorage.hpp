// ======================================================================
// \title  MessageStorage.hpp
// \author Marius Baden
// \brief  hpp file for MessageStorage component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef MessageStorage_HPP
#define MessageStorage_HPP

#include <mutex>
#include <deque>
#include <functional>

#include <Os/File.hpp>

#include "SpacePosts/MessageStorage/MessageStorageComponentAc.hpp"

namespace SpacePosts
{
  // ----------------------------------------------------------------------
  // Local Type Definitions
  // ----------------------------------------------------------------------

  // Make error stage enums availble in this scope
  typedef MessageStorage_MessageWriteError MessageWriteError;
  typedef MessageStorage_MessageReadError MessageReadError;
  typedef MessageStorage_IndexRestoreError IndexRestoreError;

  // Anonymous namespace for local buffer.
  // Marius Baden: This is how the framework implements it in PrmDbImpl.cpp
  //  	Not sure why a general stack buffer is not declared somewhere else
  // 		as a reusable type.
  namespace
  {
    // Buffer on stack for serializing the message before writing it to file
    // This avoids allocating memory on the heap
    class StackBuffer : public Fw::SerializeBufferBase
    {
    private:
      // Has to be defined so that CAPACITY = max(max encoding length, max decoding length) for all encoding and
      // decoding operations that this StackBuffer class is used for.
      const static U32 CAPACITY = SpacePosts::SpacePost::SERIALIZED_SIZE;

    public:
      NATIVE_UINT_TYPE getBuffCapacity() const
      {
        return sizeof(m_buff);
      }

      U8 *getBuffAddr()
      {
        return m_buff;
      }

      const U8 *getBuffAddr() const
      {
        return m_buff;
      }

      //! Serializes the given serializable to this buffer by calling the serialize() method.
      //!
      //! Asserts that the serialization was successful and resets the buffer before serializing.
      //!
      //! The template parameter T must be a type for which an overloading of
      //! Fw::SerializeBufferBase.serialize(const T &val) exists
      template <typename T>
      void safeSerialize(
          const T &serizable /*!< The serializable to be serialized */
      )
      {
        this->resetSer();
        Fw::SerializeStatus serialize_status = this->serialize(serizable);

        // Serialization should always be successful: We only write to the stack
        FW_ASSERT(serialize_status == Fw::FW_SERIALIZE_OK, static_cast<NATIVE_INT_TYPE>(serialize_status));
      }

      //! Deserializes the given buffer into the given variable by calling the deserialize() method.
      //!
      //! Sets the given read_size, and resets the buffer before
      //! deserializing.
      //!
      //! Calls the provided lambdas if an error occurs during deserialization.
      //!
      //! The template parameter T must be a type for which an overloading of
      //! Fw::SerializeBufferBase.deserialize(T &val) exists
      template <typename T>
      void safeDeserialize(
          T &deserialization_target,                                                      /*< The variable into which to deserialize the buffer */
          const NATIVE_UINT_TYPE read_size,                                               /*< The number of bytes that are valid in this buffer and
                                                                                              can be deserialized */
          const std::function<void()> trigger_set_length_failure,                         /*< The function to call if setting the buffer length fails.
                                                                                      Used only in this error case. */
          std::function<void(const NATIVE_UINT_TYPE)> trigger_deserialize_status_failure, /*!< The function to call if the deserialization of the
                                                              buffer into the provided variable fails.
                                                              Used only in this error case. */
          std::function<void(const NATIVE_UINT_TYPE)> trigger_deserialize_length_failure  /*!< The function to call if the deserialization of the
                                                     buffer into the provided variable fails.
                                                     Used only in this error case. */
      )
      {
        Fw::SerializeStatus deserialize_status = this->setBuffLen(read_size);

        if (deserialize_status != Fw::FW_SERIALIZE_OK)
        {
          trigger_set_length_failure();
        }

        this->resetDeser();
        deserialize_status = this->deserialize(deserialization_target);

        if (deserialize_status != Fw::FW_SERIALIZE_OK)
        {
          trigger_deserialize_status_failure(deserialize_status);
        }

        // Check whether deserialized type used all of the available data in this buffer.
        // Necessary to check because deserialization does not fail if the deserialized type is shorter than the
        // provided data in this buffer.
        // Can occur, for example, if deserializing a string and the message string contains
        // a null terminator in the middle.
        if (this->getBuffLeft() != 0)
        {
          trigger_deserialize_length_failure(this->getBuffLeft());
        }
      }

    private:
      U8 m_buff[CAPACITY];
    };
  }

  class MessageStorage : public MessageStorageComponentBase
  {

  private:
    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    // Counter to keep track of the next index to use when storing a message.
    //
    // This is used to ensure that messages are stored in consecutive indices.
    //
    // Is restored from the highest index found in the configured storage directory upon startup.
    U32 nextIndexCounter = 0;

    // The number of attempts made to store a message using this component
    U32 numStoreAttempts = 0;

    // The number of attempts made to load a message using this component
    U32 numLoadAttempts = 0;

    //! An indexing data structure to keep track of for which indices a message is successfully stored in the storage
    //! directory.
    //!
    //! Stores only up to the last N of such indices where N = MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE.
    //! I.e., lastSuccessfullyStoredIndices.size() in [0, N].
    std::deque<U32> lastSuccessfullyStoredIndices;

    // ----------------------------------------------------------------------
    // Private member functions
    // ----------------------------------------------------------------------

    //! Stores a message at the the provided index.
    //!
    //! If a file already exists at the provided index, storing the message will fail.
    //!
    //! Returns true if the message was successfully stored, false otherwise.
    //!
    //! If the message was not successfully stored, an event of type
    //! MessageStorage_MessageWriteError is triggered.
    //!
    bool storeMessage(
        const U32 index,             /*!< The index at which to store the message */
        const Fw::Serializable &data /*!< The content of the message to be stored */
    );

    //! Loads a message with the provided index if it exists in the storage directory.
    //!
    //! If no file exists at the provided index, loading the message will fail.
    //!
    //! Returns true if the message was successfully loaded, false otherwise.
    //! In the latter case, the passed SpacePost object is not modified.
    //!
    //! If the message was not successfully loaded, an event of type
    //! MessageStorage_MessageReadError is triggered. The given SpacePost object may
    //! have been modfiied and is not guaranteed to be in a valid state.
    bool loadMessage(
        const U32 index,       /*!< The index at which to load the message */
        Fw::Serializable &data /*!< The SpacePost object which will be loaded from file. The variable behind the
                                    the given reference will be overwritten with the loaded SpacePost (if loading
                                    was successful) */
    );

    //! Sets the next index to 1 + (the highest index found in the configured storage directory).
    //!
    //! This is needed after every restart to ensure that the components continues counting
    //! up the index from where it has left off. Otherwise, the component would try to overwrite
    //! existing files.
    //!
    //! Returns true if a stored SpacePost was found and the nextIndexCounter was thus set
    //! to the subsequent index.
    bool restoreIndexFromHighestStoredIndexFoundInDirectory();

    //! Gets the next index at which a message can be stored.
    //! and advances the index counter.
    //!
    //! Is thread-safe: multiple threads can call this function at the same time.
    //! Each of them will get a unique index.
    //!
    //! Emits telemetry message with the next index.
    U32 nextIndex();

    //! Puts an index into the lastSuccessfullyStoredIndices data structure to remember that a message was successfully
    //! stored at this index.
    //!
    //! This methods encapsulates the logic of how to update the data structure when a new index is added (e.g.,
    //! remove the oldest index if the data structure is full).
    void addIndexToLastSuccessfullyStoredIndices(const U32 index);

    //! Gets the absolute file path for storing a message when wanting to store it at the given index.
    std::string indexToAbsoluteFilePath(const U32 index);

    //! Writes the given buffer to the given file.
    //!
    //! Buffer is given by providing a pointer to it. The buffer content is not changed.
    //!
    //! To be used with caution! Only supposed to be called after checks for valid open file
    //! and valid buffer serialization have been performed.
    //!
    //! Returns regularly iff the buffer was successfully written to the file.
    //! Otherwise, throws a MessageWriteError as exception.
    //!
    //! If the buffer was not successfully written to the file, an event of type
    //! MessageStorage_MessageWriteError is triggered.
    //!
    //! The expected_write_size is asserted to be equal to the length of the passed buffer
    void writeRawBufferToFile(
        const void *const buffer_address,          /*!< The buffer to be written to the file */
        Os::File &file,                            /*!< The file to which the buffer should be written */
        const NATIVE_INT_TYPE write_size,          /*!< The number of bytes to read from the buffer and write to the
                                                         file */
        const U32 &index,                          /*!< The index of the current storing process.
                                                         Used only for error message upon fail */
        const MessageWriteError write_error_stage, /*!< The error type to include in the error event if the write fails.
                                                         Used only for error message upon fail */
        const MessageWriteError size_error_stage   /*!< The error type to include in the error event if the written
                                                         size does not match the expected size.
                                                         Used only for error message upon fail */
    );

    //! Reads a given number of bytes from the given file into the given buffer.
    //!
    //! Buffer is given by a pointer to it. Buffer must be allocated for at least read_size bytes. Buffer
    //! content is overwritten.
    //!
    //! To be used with caution! Only supposed to be called after checks for valid open file.
    //!
    //! Returns regularly iff the file was successfully read into the buffer.
    //! Otherwise, throws a MessageReadError as exception.
    //!
    //! The given read_size is asserted to be equal to the number of bytes in fact read from the file.
    void readRawBufferFromFile(
        void *const buffer_address,              /*!< The buffer to which to write the read file content */
        Os::File &file,                          /*!< The file from which data should be read into the buffer */
        const NATIVE_INT_TYPE read_size,         /*!< The number of bytes to read from the file and write to the
                                                       buffer */
        const U32 &index,                        /*!< The index of the message currently being loaded.
                                                       Used only for error message upon fail */
        const MessageReadError read_error_stage, /*!< The error type to include in the error event if the read fails.
                                                       Used only for error message upon fail */
        const MessageReadError size_error_stage  /*!< The error type to include in the error event if the read size
                                                       does not match the expected size.
                                                       Used only for error message upon fail */
    );

    //! Convenience method for writing a SerializeBufferBase to a file via writeRawBufferToFile()
    //!
    //! Just calls writeRawBufferToFile() with the correct parameters. I.e., the StackBuffer's address and size.
    //!
    //! The expected_write_size is asserted to be equal to the length of the passed buffer
    void writeSerializeBufferToFile(
        Fw::SerializeBufferBase &buffer,           /*!< The buffer to be written to the file */
        Os::File &file,                            /*!< The file to which the buffer should be written */
        const NATIVE_INT_TYPE expected_write_size, /*!< The expected size to be written to the file */
        const U32 &index,                          /*!< The index of the current storing process.
                                                         Used only for error message upon fail */
        const MessageWriteError write_error_stage, /*!< The error type to include in the error event if the write fails.
                                                         Used only for error message upon fail */
        const MessageWriteError size_error_stage   /*!< The error type to include in the error event if the written
                                                         size does not match the expected size.
                                                         Used only for error message upon fail */
    );

    //! Checks whether the configured storage directory (MESSAGESTORAGE_MSGFILE_DIRECTORY) exists and creates it
    //! if it does not exist.
    //!
    //! Reports a case of a non-existing directory as a STORAGE_DIRECTORY_WARNING event.
    //!
    //! Possibly, creating the directory fails. This is also reported as a STORAGE_DIRECTORY_WARNING event.
    void createStorageDirectoryIfNotExists();

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object MessageStorage
    //!
    MessageStorage(
        const char *const compName /*!< The component name*/
    );

    //! Initialize object MessageStorage
    //!
    void init(
        const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
    );

    //! Destroy object MessageStorage
    //!
    ~MessageStorage();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for storeMessage
    //!
    //! Stores the given message at the next available index.
    //!
    //! Does not make any assumptions about the members of BSSMessage. I.e., only handles it as a Serializeable type.
    //!
    //! If the message was not successfully stored:
    //!   - an event of type
    //!     MessageStorage_MessageWriteError is triggered.
    //!   - The index at which the message was attempted to be stored will not be
    //!     reused until the next time the indexing is reset.
    //!
    //! Is thread-safe: multiple threads can call this function at the same time.
    SpacePosts::MessageStorageStatus storeMessage_handler(
        const NATIVE_INT_TYPE portNum, /*!< The port number*/
        const SpacePosts::SpacePost &data     /*!< the content of the message */
        ) override;

    //! Handler implementation for loadMessage
    //!
    //! Does not make any assumptions about the members of BSSMessage. I.e., only handles it as a Serializeable type.
    SpacePosts::SpacePostValid loadMessageFromIndex_handler(
        const NATIVE_INT_TYPE portNum, /*!< The port number*/
        const U32 index,               /*!< The index of the message to be loaded */
        SpacePosts::SpacePost &data           /*!< The content of the message */
        ) override;

    //! Handler implementation for loadMessageLastN
    //!
    //! Uses the lastSuccessfullyStoredIndices data structure to determine the indices of the last messages to be
    //! loaded.
    //!
    //! Does not make any assumptions about the members of BSSMessage. I.e., only handles it as a Serializeable type.
    U8 loadMessageLastN_handler(
        const NATIVE_INT_TYPE portNum,     /*!< The port number*/
        U8 num_messages,                   /*!< The number of messages to load */
        SpacePosts::SpacePost_Batch &lastMessages /*!< The content of the message */
        ) override;
  };

} // end namespace SpacePosts

#endif
