// ======================================================================
// \title  MessageStorage.cpp
// \author Marius Baden
// \brief  cpp file for MessageStorage component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================
#include <string>
#include <vector>
#include <functional>

#include <Os/File.hpp>
#include <Os/Directory.hpp>
#include <Os/FileSystem.hpp>

#include <SpacePosts/MessageStorage/MessageStorage.hpp>
#include "SpacePosts/MessageTypes/FppConstantsAc.hpp"
#include <config/MessageStorageCfg.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <MessageStorage.hpp>

namespace SpacePosts
{
	// ----------------------------------------------------------------------
	// Construction, initialization, and destruction
	// ----------------------------------------------------------------------

	MessageStorage ::
		MessageStorage(
			const char *const compName)
		: MessageStorageComponentBase(compName),
		  nextIndexCounter(0),
		  lastSuccessfullyStoredIndices()
	{
	}

	void MessageStorage ::
		init(
			const NATIVE_INT_TYPE instance)
	{
		MessageStorageComponentBase::init(instance);
		this->restoreIndexFromHighestStoredIndexFoundInDirectory();
	}

	MessageStorage ::
		~MessageStorage()
	{
	}

	// ----------------------------------------------------------------------
	// Handler implementations for user-defined typed input ports
	// ----------------------------------------------------------------------

	SpacePosts::MessageStorageStatus MessageStorage ::
		storeMessage_handler(
			const NATIVE_INT_TYPE portNum,
			const SpacePosts::SpacePost &data)
	{
		const U32 index = this->nextIndex();

		this->tlmWrite_STORE_COUNT(++this->numStoreAttempts);

		const bool success = this->storeMessage(index, data);

		const SpacePosts::MessageStorageStatus status = success
															? SpacePosts::MessageStorageStatus::OK
															: SpacePosts::MessageStorageStatus::ERROR;
		return status;
	}

	SpacePosts::SpacePostValid MessageStorage ::
		loadMessageFromIndex_handler(
			const NATIVE_INT_TYPE portNum,
			const U32 index,
			SpacePosts::SpacePost &data)
	{
		const bool success = this->loadMessage(index, data);
		this->tlmWrite_LOAD_COUNT(++this->numLoadAttempts);
		const SpacePosts::SpacePostValid status = static_cast<SpacePosts::SpacePostValid::t>(success);
		return status;
	}

	U8 MessageStorage ::
		loadMessageLastN_handler(const NATIVE_INT_TYPE portNum, const U8 num_messages, SpacePosts::SpacePost_Batch &lastMessages)
	{
		U8 num_messages_to_load{num_messages};
		if (SpacePost_Batch_Size < num_messages_to_load)
		{
			num_messages_to_load = SpacePost_Batch_Size;
		}

		SpacePosts::SpacePost_Array messages_batch{}; // New array to write info to

		// Get iterator of lastSuccessfullyStoredIndices pointing from the back to the first index to load
		auto iterator = this->lastSuccessfullyStoredIndices.crbegin();
		int num_messages_loaded{0};
		while (num_messages_loaded < num_messages_to_load && iterator != this->lastSuccessfullyStoredIndices.crend())
		{
			const U32 index_to_load = *iterator;
			SpacePosts::SpacePost &message_to_load_into = messages_batch[num_messages_loaded];
			const bool success = this->loadMessage(index_to_load, message_to_load_into);
			this->tlmWrite_LOAD_COUNT(++this->numLoadAttempts);

			if (success)
			{
				++num_messages_loaded;
			}
			++iterator;
		}

		lastMessages.setmessages(messages_batch); // Write dirty copy back
		lastMessages.setnumValidMessages(num_messages_loaded);
		return num_messages_loaded;
	}

	// ----------------------------------------------------------------------
	// Private member functions
	// ----------------------------------------------------------------------

	bool MessageStorage ::
		storeMessage(const U32 index, const Fw::Serializable &data)
	{
		StackBuffer stackBuff{};
		Os::File::Status file_op_status;

		const std::string file_name_absolute = this->indexToAbsoluteFilePath(index);

		this->createStorageDirectoryIfNotExists();

		try // Intentionally large try block to handle abort due to all kinds of MessageWriteError exceptions equally
		{
			/*
			 *	Open file
			 */
			// Check whether file exists: file is not automatically created when opening for read
			Os::File testExistsFile{};
			file_op_status = testExistsFile.open(file_name_absolute.c_str(), Os::File::OPEN_READ);
			if (file_op_status != Os::File::DOESNT_EXIST)
			{
				this->log_WARNING_HI_MESSAGE_STORE_FAILED(index, MessageWriteError::FILE_EXISTS, file_op_status);

				throw MessageWriteError(MessageWriteError::FILE_EXISTS);
			}

			// File is automatically created when opening for write
			Os::File file{};
			file_op_status = file.open(file_name_absolute.c_str(), Os::File::OPEN_SYNC_WRITE);
			if (file_op_status != Os::File::OP_OK)
			{
				this->log_WARNING_HI_MESSAGE_STORE_FAILED(index, MessageWriteError::OPEN, file_op_status);
				throw MessageWriteError(MessageWriteError::OPEN);
			}

			/*
			 * Write delimiter
			 */
			U8 delimiter = MESSAGESTORAGE_MSGFILE_DELIMITER;
			NATIVE_INT_TYPE write_size = sizeof(delimiter);
			this->writeRawBufferToFile(&delimiter, file, write_size, index,
									   MessageWriteError::DELIMITER_WRITE,
									   MessageWriteError::DELIMITER_SIZE);

			/*
			 *	Write message size = length of message type
			 */
			// Serialize message 1st time just to get its size
			stackBuff.safeSerialize(data);
			const U32 message_size = stackBuff.getBuffLength();

			stackBuff.safeSerialize(message_size);
			write_size = sizeof(message_size);
			this->writeSerializeBufferToFile(stackBuff, file, write_size, index,
											 MessageWriteError::MESSAGE_SIZE_WRITE,
											 MessageWriteError::MESSAGE_SIZE_SIZE);

			/*
			 *	Write message
			 */
			// Serialize message 2nd time to write it to file
			stackBuff.safeSerialize(data);
			write_size = message_size;
			this->writeSerializeBufferToFile(stackBuff, file, write_size, index,
											 MessageWriteError::MESSAGE_CONTENT_WRITE,
											 MessageWriteError::MESSAGE_CONTENT_SIZE);

			/*
			 *	Done
			 */
			this->addIndexToLastSuccessfullyStoredIndices(index);
			this->log_ACTIVITY_LO_MESSAGE_STORE_COMPLETE(index);
			return true;
			// file and stackBuffer are closed / deallocated automatically by their destructors
		}
		catch (const MessageWriteError &e) // This can only be triggered by the explicity throw statements above
		{
			/*
			 * Clean Up upon fail
			 */
			// Delete file if file was created but storing failed
			if (e != MessageWriteError::FILE_EXISTS)
			{
				Os::FileSystem::Status delete_status = Os::FileSystem::removeFile(file_name_absolute.c_str());
				if (delete_status != Os::FileSystem::OP_OK)
				{
					this->log_WARNING_HI_MESSAGE_STORE_FAILED(index, MessageWriteError::CLEANUP_DELETE, delete_status);
				};
			}

			return false;
		}
	}

	bool MessageStorage::loadMessage(const U32 index, Fw::Serializable &data)
	{
		StackBuffer stackBuff{};
		Os::File::Status file_op_status;
		NATIVE_INT_TYPE read_size;

		const std::string file_name_absolute = this->indexToAbsoluteFilePath(index);

		try // Same method as for storeMessage(): Throw + catch MessageReadError if an operation fails
		{
			/*
			 *	Open file
			 */
			Os::File file{};
			file_op_status = file.open(file_name_absolute.c_str(), Os::File::OPEN_READ);
			if (file_op_status != Os::File::OP_OK)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::OPEN, file_op_status);
				throw MessageReadError(MessageReadError::OPEN);
			}

			/*
			 *	Read delimiter + check whether it is the expected MESSAGESTORAGE_MSGFILE_DELIMITER
			 */
			U8 delimiter;
			this->readRawBufferFromFile(&delimiter, file, sizeof(delimiter), index,
										MessageReadError::DELIMITER_READ,
										MessageReadError::DELIMITER_SIZE);

			if (delimiter != MESSAGESTORAGE_MSGFILE_DELIMITER)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::DELIMITER_CONTENT, delimiter);
				throw MessageReadError(MessageReadError::DELIMITER_CONTENT);
			}

			/*
			 *	Read message size
			 */
			U32 message_size{0};
			read_size = sizeof(message_size);
			this->readRawBufferFromFile(stackBuff.getBuffAddr(), file, read_size, index,
										MessageReadError::MESSAGE_SIZE_READ,
										MessageReadError::MESSAGE_SIZE_SIZE);

			const std::function<void()> set_length_failure_callback_size = [&]()
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_SIZE_DESER_SET_LENGTH,
														 message_size);
				throw MessageReadError(MessageReadError::MESSAGE_SIZE_DESER_SET_LENGTH);
			};
			const std::function<void(const NATIVE_UINT_TYPE)> deserialize_status_failure_callback_size = [&](const NATIVE_UINT_TYPE error_code)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_SIZE_DESER_EXCECUTE,
														 error_code);
				throw MessageReadError(MessageReadError::MESSAGE_SIZE_DESER_EXCECUTE);
			};
			const std::function<void(const NATIVE_UINT_TYPE)> deserialize_length_failure_callback_size = [&](const NATIVE_UINT_TYPE error_code)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH,
														 error_code);
				throw MessageReadError(MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH);
			};
			stackBuff.safeDeserialize(message_size, read_size, set_length_failure_callback_size,
									  deserialize_status_failure_callback_size,
									  deserialize_length_failure_callback_size);

			// Check whether message will fit into stackBuff
			if (message_size > stackBuff.getBuffCapacity())
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_SIZE_EXCEEDS_BUFFER,
														 message_size);
				throw MessageReadError(MessageReadError::MESSAGE_SIZE_EXCEEDS_BUFFER);
			}

			// Check whether message size is 0. Could not procede if it is, because we would try to read 0 bytes
			// for the message content's serialization
			if (message_size == 0)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_SIZE_ZERO, 0);
				throw MessageReadError(MessageReadError::MESSAGE_SIZE_ZERO);
			}

			/*
			 *	Read message
			 */
			read_size = message_size;
			this->readRawBufferFromFile(stackBuff.getBuffAddr(), file, read_size, index,
										MessageReadError::MESSAGE_CONTENT_READ,
										MessageReadError::MESSAGE_CONTENT_SIZE);

			const std::function<void()> set_length_failure_callback_content = [&]()
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_CONTENT_DESER_SET_LENGTH,
														 read_size);
				throw MessageReadError(MessageReadError::MESSAGE_CONTENT_DESER_SET_LENGTH);
			};
			const std::function<void(const NATIVE_UINT_TYPE)> deserialize_status_failure_callback_content = [&](const NATIVE_UINT_TYPE error_code)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
														 error_code);
				throw MessageReadError(MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE);
			};
			const std::function<void(const NATIVE_UINT_TYPE)> deserialize_length_failure_callback_content = [&](const NATIVE_UINT_TYPE error_code)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH,
														 error_code);
				throw MessageReadError(MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH);
			};
			stackBuff.safeDeserialize(data, read_size, set_length_failure_callback_content,
									  deserialize_status_failure_callback_content,
									  deserialize_length_failure_callback_content);

			/*
			 *	Done
			 */

			// Check whether file has been read to the end by trying to read one more byte and expecting it to read none
			read_size = 1;
			const Os::File::Status file_status = file.read(stackBuff.getBuffAddr(), read_size, true);
			if (file_status != Os::File::OP_OK || read_size != 0)
			{
				this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, MessageReadError::FILE_END, file_status);
				throw MessageReadError(MessageReadError::FILE_END);
			}

			this->log_ACTIVITY_LO_MESSAGE_LOAD_COMPLETE(index);
			return true;
		}
		catch (const MessageReadError &e)
		{
			// Put common handling of file open errors here if needed
			// Try-catch is kept, even though it is empty for now, for consistency with storeMessage()

			return false;
		}
	}

	bool MessageStorage::restoreIndexFromHighestStoredIndexFoundInDirectory()
	{
		this->createStorageDirectoryIfNotExists();

		Os::Directory storage_dir;
		Os::Directory::Status dir_status;

		dir_status = storage_dir.open(MESSAGESTORAGE_MSGFILE_DIRECTORY.c_str());
		if (dir_status != Os::Directory::OP_OK)
		{
			this->log_WARNING_HI_INDEX_RESTORE_FAILED(IndexRestoreError::STORAGE_DIR_OPEN, dir_status);
			return false;
		}

		const U32 &readSize = MESSAGESTORAGE_MSGFILE_NAME_MAXLENGTH;
		char file_name[readSize + 1]; // +1 to have space for null terminator
		file_name[readSize] = '\0';	  // Set null terminator to make sure that string is always terminated even after read
		std::vector<std::string> existing_file_names{};
		do
		{
			dir_status = storage_dir.read(file_name, readSize);
			if (dir_status == Os::Directory::OP_OK)
			{
				existing_file_names.push_back(std::string{file_name});
			}
		} while (dir_status == Os::Directory::OP_OK);

		// Trigger warning event if directory read finished with error instead of reaching the end of the directory
		if (dir_status != Os::Directory::NO_MORE_FILES)
		{
			this->log_WARNING_HI_INDEX_RESTORE_FAILED(IndexRestoreError::STORAGE_DIR_READ, dir_status);
			return false; // Fail early: We could continue here but chose to fail
		}

		// Iterate over existing_file_names and remove ".spacepost" suffix from each string
		std::vector<U32> existing_file_indices{};
		for (std::string &file_name : existing_file_names)
		{
			// Check for correct format with REGEX
			if (std::regex_match(file_name, MESSAGESTORAGE_MSGFILE_FILE_NAME_REGEX) == false)
			{
				// Not parsable. Try next file_name
				continue;
			}

			// Remove suffix
			file_name.erase(file_name.length() - MESSAGESTORAGE_MSGFILE_FILE_EXTENSION.length(),
							MESSAGESTORAGE_MSGFILE_FILE_EXTENSION.length());

			// Parse remaining string to U32
			U32 parsed_index{0};
			try
			{
				parsed_index = std::stoul(file_name);
			}
			catch (const std::exception &e) // Catch all exceptions to prevent crash
			{
				// Not parsable. Try next file_name
				continue;
			}

			existing_file_indices.push_back(static_cast<U32>(parsed_index));
		}

		// Restore lastSuccessfullyStoredIndices from the parsed indices
		std::sort(existing_file_indices.begin(), existing_file_indices.end());
		this->lastSuccessfullyStoredIndices.clear();
		const std::size_t restored_history_size = std::min(
			static_cast<size_t>(MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE), existing_file_indices.size());
		this->lastSuccessfullyStoredIndices.resize(restored_history_size);
		std::copy(existing_file_indices.cend() - restored_history_size,
				  existing_file_indices.cend(),
				  this->lastSuccessfullyStoredIndices.begin());

		// Restore nextIndexCounter from last index in existing_file_indices
		if (existing_file_indices.empty())
		{
			// No files found
			this->nextIndexCounter = MESSAGESTORAGE_INITIAL_INDEX;
			this->log_ACTIVITY_LO_INDEX_RESTORE_COMPLETE(0, 0);
		}
		else
		{
			this->nextIndexCounter = existing_file_indices.back() + 1;
			this->log_ACTIVITY_LO_INDEX_RESTORE_COMPLETE(existing_file_indices.size(), this->nextIndexCounter - 1);
		}

		this->tlmWrite_NEXT_STORAGE_INDEX(this->nextIndexCounter);

		return true;
	}

	U32 MessageStorage ::
		nextIndex()
	{
		const U32 &index = this->nextIndexCounter++;

		// Handle index wrap around
		if (this->nextIndexCounter == 0)
		{
			this->log_WARNING_LO_INDEX_WRAP_AROUND();
		}

		this->tlmWrite_NEXT_STORAGE_INDEX(index + 1);

		return index;
	}

	void MessageStorage::addIndexToLastSuccessfullyStoredIndices(const U32 index)
	{
		if (this->lastSuccessfullyStoredIndices.size() >= MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE)
		{
			this->lastSuccessfullyStoredIndices.pop_front();
		}
		this->lastSuccessfullyStoredIndices.push_back(index);
	}

	std::string MessageStorage::indexToAbsoluteFilePath(const U32 index)
	{
		return MESSAGESTORAGE_MSGFILE_DIRECTORY +
			   std::to_string(index) +
			   MESSAGESTORAGE_MSGFILE_FILE_EXTENSION;
	}

	void MessageStorage::writeSerializeBufferToFile(Fw::SerializeBufferBase &serializeBuffer, Os::File &file,
													const NATIVE_INT_TYPE expected_write_size, const U32 &index,
													const MessageWriteError write_error_stage,
													const MessageWriteError size_error_stage)
	{
		NATIVE_INT_TYPE actual_write_size = serializeBuffer.getBuffLength();
		FW_ASSERT(actual_write_size == expected_write_size, actual_write_size);
		this->writeRawBufferToFile(serializeBuffer.getBuffAddr(), file, actual_write_size,
								   index, write_error_stage, size_error_stage);
	}

	void MessageStorage::writeRawBufferToFile(const void *const buffer_address, Os::File &file,
											  const NATIVE_INT_TYPE expected_write_size, const U32 &index,
											  const MessageWriteError write_error_stage,
											  const MessageWriteError size_error_stage)
	{
		NATIVE_INT_TYPE write_size{expected_write_size}; // Might be overwritten by file.write()
		const Os::File::Status file_op_status = file.write(buffer_address, write_size, true);
		if (file_op_status != Os::File::OP_OK)
		{
			this->log_WARNING_HI_MESSAGE_STORE_FAILED(index, write_error_stage, file_op_status);
			throw write_error_stage;
		}

		// file.write() overwrites write_size with the number of bytes actually written
		if (write_size != expected_write_size)
		{
			this->log_WARNING_HI_MESSAGE_STORE_FAILED(index, size_error_stage, write_size);
			throw size_error_stage;
		}
	}

	void MessageStorage::readRawBufferFromFile(void *const buffer_address, Os::File &file,
											   const NATIVE_INT_TYPE expected_read_size, const U32 &index,
											   const MessageReadError read_error_stage,
											   const MessageReadError size_error_stage)
	{
		NATIVE_INT_TYPE read_size{expected_read_size}; // Will be overwritten by file.read()
		const Os::File::Status file_op_status = file.read(buffer_address, read_size, true);
		if (file_op_status != Os::File::OP_OK)
		{
			this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, read_error_stage, file_op_status);
			throw read_error_stage;
		}

		// file.read() overwrites read_size with the number of bytes actually read
		if (read_size != expected_read_size)
		{
			this->log_WARNING_LO_MESSAGE_LOAD_FAILED(index, size_error_stage, read_size);
			throw size_error_stage;
		}
	}

	void MessageStorage::createStorageDirectoryIfNotExists()
	{
		Os::FileSystem::Status dir_create_status = Os::FileSystem::createDirectory(
			MESSAGESTORAGE_MSGFILE_DIRECTORY.c_str());
		if (dir_create_status != Os::FileSystem::ALREADY_EXISTS)
		{
			const bool creation_successful{dir_create_status == Os::FileSystem::OP_OK};
			this->log_WARNING_LO_STORAGE_DIRECTORY_WARNING(MESSAGESTORAGE_MSGFILE_DIRECTORY.c_str(),
														   creation_successful);
		}
	}

} // end namespace SpacePosts
