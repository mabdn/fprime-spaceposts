// ======================================================================
// \title  MessageStorage.hpp
// \author marius
// \brief  cpp file for MessageStorage test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <iostream>

#include "STest/STest/testing.hpp"

#include "config/MessageStorageCfg.hpp"
#include "STest/Pick/Pick.hpp"
#include "Os/FileSystem.hpp"
#include "Os/Directory.hpp"
#include "Os/Stubs/FileStubs.hpp"

#include "Tester.hpp"
#include "SpacePosts/MessageStorage/MessageStorage.hpp"
#include "SpacePosts/MessageTypes/FppConstantsAc.hpp"
#include "model/StorageDirectorySetup.hpp"
#include "model/SpacePostFile.hpp"

#define INSTANCE 0
#define MAX_HISTORY_SIZE SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size
#define QUEUE_DEPTH 10
#define TEXT_MESSAGE_LOG_SIZE 10

namespace SpacePosts
{

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  Tester ::
      Tester(const StorageDirectorySetup directorySetup) : m_directory(directorySetup),

#if FW_OBJECT_NAMES == 1
                                                           MessageStorageGTestBase("Tester", MAX_HISTORY_SIZE),
                                                           component("MessageStorage")
#else
                                                           MessageStorageGTestBase(MAX_HISTORY_SIZE),
                                                           component()
#endif
  {
    this->connectPorts();
  }

  Tester ::
      ~Tester()
  {
  }

  // ----------------------------------------------------------------------
  // Storage Tests
  // ----------------------------------------------------------------------

  void Tester::testStoreIndex()
  {
    this->realizeDirectorySetupAndInitializeComponents();
    const U32 expected_index = this->m_directory.getNextSpacePostIndex();

    // Test whether the index used for the next message is the one that was reported to have been restored
    SpacePost test_message{"some text"}; // Text does not matter here because we only check the assigned index
    MessageStorageStatus status = this->invoke_to_storeMessage(0, test_message);

    ASSERT_EQ(status.e, MessageStorageStatus::OK)
        << "Failed to store message after restoring index " << expected_index;

    // Check event for reported correct index
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_COMPLETE_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_COMPLETE(0, expected_index);

    // Check telemetry for reported correct index
    ASSERT_TLM_SIZE(2);
    ASSERT_TLM_STORE_COUNT_SIZE(1);
    ASSERT_TLM_STORE_COUNT(0, 1);
    ASSERT_TLM_NEXT_STORAGE_INDEX_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX(0, expected_index + 1);
  }

  void Tester::testStoreIndexDirDoesNotExist()
  {
    /*
      Explicit initialization to check occurence of special event: STORAGE_DIRECTORY_WARNING
    */

    const U32 expected_index = MESSAGESTORAGE_INITIAL_INDEX;
    this->m_directory.realizeOnFileSystem();

    this->init();
    this->component.init(INSTANCE);

    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_STORAGE_DIRECTORY_WARNING_SIZE(1);
    ASSERT_EVENTS_STORAGE_DIRECTORY_WARNING(0, MESSAGESTORAGE_MSGFILE_DIRECTORY.c_str(), true);
    ASSERT_EVENTS_INDEX_RESTORE_COMPLETE_SIZE(1);
    ASSERT_EVENTS_INDEX_RESTORE_COMPLETE(0, 0, 0);

    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX(0, expected_index);

    this->clearHistory();

    /*
      Test whether storing is succesfull after component created the storage directory
     */

    SpacePost test_message{"some text"};
    MessageStorageStatus status = this->invoke_to_storeMessage(0, test_message);

    ASSERT_EQ(status.e, MessageStorageStatus::OK)
        << "Failed to store message after having to create the storage directory " << expected_index;

    // Check event for reported correct index: No STORAGE_DIRECTORY_WARNING event additionally expected
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_COMPLETE_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_COMPLETE(0, expected_index);

    ASSERT_TLM_SIZE(2);
    ASSERT_TLM_STORE_COUNT_SIZE(1);
    ASSERT_TLM_STORE_COUNT(0, 1);
    ASSERT_TLM_NEXT_STORAGE_INDEX_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX(0, expected_index + 1);
  }

  void Tester::testStoreMessageText(std::string message_text)
  {
    this->realizeDirectorySetupAndInitializeComponents();
    const U32 expected_index = this->m_directory.getNextSpacePostIndex();

    SpacePost test_message{message_text.c_str()};
    MessageStorageStatus status = this->invoke_to_storeMessage(0, test_message);
    ASSERT_EQ(status.e, MessageStorageStatus::OK)
        << "Failed to store message: " << message_text;

    // Check stored file for correctness
   SpacePostFile file{};
    file.readFromStorageDirectory(expected_index);
    this->expectSpacePostFileCorrectForMessage(file, test_message);

    // Check that other files were not changed
    this->m_directory.expectAllSpacePostFilesAreOnDiskAndAreUnchanged();
  }

  void Tester::testLoadFromExistingIndex(const U32 index, const U32 num_messages_loaded)
  {
    this->realizeDirectorySetupAndInitializeComponents();

    SpacePost loaded_message{};
    SpacePostValid result_status = this->invoke_to_loadMessageFromIndex(0, index, loaded_message);
    ASSERT_EQ(result_status.e, SpacePostValid::VALID)
        << "Failed to load message from index " << index << " even though it points to a valid message file";

    // Check that successful loading was reported via events and telemetry
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE(0, index);

    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_LOAD_COUNT_SIZE(1);
    ASSERT_TLM_LOAD_COUNT(0, num_messages_loaded + 1);
  }

  void Tester::testLoadFromNonExistingIndex(const U32 index)
  {
    this->realizeDirectorySetupAndInitializeComponents();

    SpacePost loaded_message{};
    SpacePostValid result_status = this->invoke_to_loadMessageFromIndex(0, index, loaded_message);
    ASSERT_EQ(result_status.e, SpacePostValid::INVALID)
        << "Load from index " << index << " should have failed because no message file exists for that index";

    // Check that failed loading was reported via events and telemetry
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_FAILED_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_FAILED(0, index, MessageReadError::OPEN, static_cast<I32>(Os::File::Status::DOESNT_EXIST));

    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_LOAD_COUNT_SIZE(1);
    ASSERT_TLM_LOAD_COUNT(0, 1);
  }

  void Tester::testLoadValidSpacePostFileFromIndex(const U32 index,SpacePostFile file_to_load)
  {
    this->m_directory.addSpacePostFile(index, file_to_load);
    this->realizeDirectorySetupAndInitializeComponents();

    SpacePost loaded_message{};
    SpacePostValid result_status = this->invoke_to_loadMessageFromIndex(0, index, loaded_message);

    ASSERT_EQ(result_status.e, SpacePostValid::VALID)
        << "Loading message from index " << index << " failed even though the message file at that index is valid"
        << std::endl
        << "Message content: " << file_to_load.getMessageText();

    // Check message content is the one that was stored
    this->expectSpacePostFileCorrectForMessage(file_to_load, loaded_message);

    // Check that all files were not changed
    this->m_directory.expectAllSpacePostFilesAreOnDiskAndAreUnchanged();
  }

  void Tester::testLoadInvalidSpacePostFileFromIndex(const U32 index, constSpacePostFile spacePostFile,
                                               const MessageReadError expectedErrorStage, const I32 expectedErrorCode)
  {
    this->m_directory.addSpacePostFile(index, spacePostFile);
    this->realizeDirectorySetupAndInitializeComponents();

    SpacePost loaded_message{};
    SpacePostValid result_status = this->invoke_to_loadMessageFromIndex(0, index, loaded_message);

    ASSERT_EQ(result_status.e, SpacePostValid::INVALID)
        << "Loading SpacePost from invalid file " << spacePostFile << " should have failed" << std::endl
        << "Loaded from index " << index;

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_FAILED_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_FAILED(0, index, expectedErrorStage, expectedErrorCode);

    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_LOAD_COUNT_SIZE(1);

    // Check that all files were not changed
    this->m_directory.expectAllSpacePostFilesAreOnDiskAndAreUnchanged();
  }

  void Tester::testLoadLastNMessagesExistingInDirectory(const U32 numMessagesToLoad)
  {

    this->realizeDirectorySetupAndInitializeComponents();

    // Find expected indices to load
    std::map<U32,SpacePostFile> files_expected = this->m_directory.getLastNSpacePostFiles(numMessagesToLoad);
    auto indices_expected_iter = files_expected.crbegin();

    // Could be smaller than numMessagesToLoad if directory has less than numMessagesToLoad messages
    const U32 num_messages_expected = files_expected.size();

    // Load messages from directory
    SpacePost_Batch loaded_messages{};
    const U8 numU8MessagesToLoad = numMessagesToLoad > std::numeric_limits<U8>::max()
                                       ? std::numeric_limits<U8>::max()
                                       : static_cast<U8>(numMessagesToLoad);
    U8 num_messages_loaded = this->invoke_to_loadMessageLastN(0, numU8MessagesToLoad, loaded_messages);

    ASSERT_EQ(static_cast<U32>(num_messages_loaded), num_messages_expected)
        << "Failed to load " << numMessagesToLoad << " messages from directory" << std::endl
        << "Expected " << num_messages_expected << " but loaded " << static_cast<U32>(num_messages_loaded)
        << " messages instead" << std::endl
        << "Mesages in directory: " << this->m_directory.getExistingSpacePostIndices().size() << std::endl;

    // Check that component decided to load the messages from the correct indices
    ASSERT_EVENTS_SIZE(num_messages_expected);
    ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE_SIZE(num_messages_expected);
    for (U32 i = 0; i < num_messages_expected; ++i)
    {
      ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE(i, indices_expected_iter->first);
      ++indices_expected_iter;
    }

    // Check telemetry
    ASSERT_TLM_SIZE(num_messages_expected);
    ASSERT_TLM_LOAD_COUNT_SIZE(num_messages_expected);
    for (U32 i = 0; i < num_messages_expected; ++i)
    {
      ASSERT_TLM_LOAD_COUNT(i, i + 1);
    }
  }

  void Tester::testLoadLastNMessagesGivenSpacePostFiles(const U8 numMessagesToLoad,
                                                  const std::vector<SpacePostFile> lastSpacePostFilesInStorage,
                                                  const std::vector<SpacePostFile> spacePostFilesExpectedToLoad)
  {
    // Ensure the test method's parameter assumptions
    FW_ASSERT(lastSpacePostFilesInStorage.size() >= spacePostFilesExpectedToLoad.size(),
              lastSpacePostFilesInStorage.size(), spacePostFilesExpectedToLoad.size());
    FW_ASSERT(spacePostFilesExpectedToLoad.size() <= numMessagesToLoad, spacePostFilesExpectedToLoad.size(), numMessagesToLoad);

    // Place the messages
    this->m_directory.setLastSpacePostFile(lastSpacePostFilesInStorage);
    this->realizeDirectorySetupAndInitializeComponents();

    // Load the messages
    SpacePost_Batch loaded_batch{};
    U8 num_messages_loaded = this->invoke_to_loadMessageLastN(0, numMessagesToLoad, loaded_batch);

    // Check that the correct number of messages was loaded
    ASSERT_EQ(num_messages_loaded, spacePostFilesExpectedToLoad.size())
        << "Loaded " << static_cast<U32>(num_messages_loaded) << " messages instead of "
        << spacePostFilesExpectedToLoad.size();
    ASSERT_EQ(loaded_batch.getnumValidMessages(), num_messages_loaded)
        << "The number of messages in the returned batch (" << loaded_batch.getnumValidMessages() << ") does not match"
        << " the number of messages in the port's return value (" << spacePostFilesExpectedToLoad.size() << ")";

    // Check message content is the one that was stored
    for (U32 i = 0; i < spacePostFilesExpectedToLoad.size(); i++)
    {
      this->expectSpacePostFileCorrectForMessage(spacePostFilesExpectedToLoad[i], loaded_batch.getmessages()[i]);
    }

    // Determine how many operations succeded and failed
    const U32 num_loads_success_expected = spacePostFilesExpectedToLoad.size();
    const U32 num_loads_failed_expected = lastSpacePostFilesInStorage.size() - spacePostFilesExpectedToLoad.size();
    const U32 num_loads_total_expected = num_loads_success_expected + num_loads_failed_expected;

    // Check events
    ASSERT_EVENTS_SIZE(num_loads_total_expected);
    ASSERT_EVENTS_MESSAGE_LOAD_FAILED_SIZE(num_loads_failed_expected);
    ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE_SIZE(num_loads_success_expected);
    // Indices are already checked in testLoadLastNMessagesExistingInDirectory

    // Check telemetry
    ASSERT_TLM_SIZE(num_loads_total_expected);
    ASSERT_TLM_LOAD_COUNT_SIZE(num_loads_total_expected);
    for (U32 i = 0; i < num_loads_total_expected; ++i)
    {
      ASSERT_TLM_LOAD_COUNT(i, i + 1);
    }
  }

  void Tester::testStoreFileCreateFails()
  {
    this->realizeDirectorySetupAndInitializeComponents();
    const U32 expected_storage_index = this->m_directory.getNextSpacePostIndex();

    // Use the F' framework to setup an OS interceptor to make the file creation fail.
    // Essentially, this injects a fake implementation of the Os::File::open function into the component.
    const Os::OpenInterceptor osInterceptor = [](Os::File::Status &status, const char *fileName, Os::File::Mode mode, void *ptr) -> bool
    {
      if (mode == Os::File::OPEN_READ) // Do not intercept read operations
      {
        return true; // Continue with the real implementation
      }
      else
      {
        status = Os::File::NO_SPACE;
        return false; // Does not continue with the real implementation. Returns the error code instead
      }
    };
    Os::registerOpenInterceptor(osInterceptor, static_cast<void *>(this));

    // Try to store a message
    const SpacePost message_to_store{"Test message"}; // Message text does not matter
    MessageStorageStatus status = this->invoke_to_storeMessage(0, message_to_store);

    // Check that the component reported the error
    ASSERT_EQ(status.e, MessageStorageStatus::ERROR)
        << "Storing a message should have failed because the file creation failed";

    // Check events
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_MESSAGE_STORE_FAILED_SIZE(2);
    ASSERT_EVENTS_MESSAGE_STORE_FAILED(0, expected_storage_index,
                                       MessageWriteError::OPEN, Os::File::NO_SPACE);
    // Second failure: Cannot delte the file because its creation failed
    ASSERT_EVENTS_MESSAGE_STORE_FAILED(1, expected_storage_index,
                                       MessageWriteError::CLEANUP_DELETE, Os::FileSystem::INVALID_PATH);

    // Clean up interceptor setup
    Os::clearOpenInterceptor();

    // Check that component is still functional
    this->testComponentFunctional();
  }

  void Tester::testStoreFileExists()
  {
    this->realizeDirectorySetupAndInitializeComponents();

    // Place a message at the index which will be used for storing next
    const U32 index_to_store = this->m_directory.getNextSpacePostIndex();
   SpacePostFile existing_file_at_storage_index{false}; // Generates random valid file
    this->m_directory.addSpacePostFile(index_to_store, existing_file_at_storage_index);
    this->m_directory.realizeOnFileSystem();

    // Try to store a message
    const SpacePost message_to_store{"Test message"}; // Message text does not matter
    MessageStorageStatus status = this->invoke_to_storeMessage(0, message_to_store);

    ASSERT_EQ(status.e, MessageStorageStatus::ERROR)
        << "Storing a message at index " << index_to_store << " should have failed because a file already exists there";

    // Check events
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_FAILED_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_FAILED(0, index_to_store, MessageWriteError::FILE_EXISTS, Os::File::OP_OK);

    // Check that component is still functional
    this->testComponentFunctional();
  }

  // ----------------------------------------------------------------------
  // Helper methods
  // ----------------------------------------------------------------------

  void Tester::testComponentFunctional()
  {
    /* Test storing */

    this->clearHistory();

    // Create a message to store
    constSpacePostFile test_file{false}; // Generates random valid file
    const SpacePost message_to_store{test_file.getMessageText().c_str()};

    // Store the message
    MessageStorageStatus status = this->invoke_to_storeMessage(0, message_to_store);

    ASSERT_EQ(status.e, MessageStorageStatus::OK)
        << "Failed to store message during concluding functionality check";

    // Check events
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_STORE_COMPLETE_SIZE(1);
    // Can't know index to expect

    // Check telemetry
    ASSERT_TLM_SIZE(2);
    ASSERT_TLM_STORE_COUNT_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX_SIZE(1);
    // Can't know the STORE_COUNT or index to expect

    /* Test loading */

    this->clearHistory();

    // Load the message
    SpacePost_Batch loaded_batch{};
    U8 num_messages_loaded = this->invoke_to_loadMessageLastN(0, 1, loaded_batch);

    // Check that the correct number of messages was loaded
    ASSERT_EQ(num_messages_loaded, 1) << "Loaded " << static_cast<U32>(num_messages_loaded) << " messages instead of 1";

    // Check message content is the one that was stored
    this->expectSpacePostFileCorrectForMessage(test_file, loaded_batch.getmessages()[0]);

    // Check events
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_MESSAGE_LOAD_COMPLETE_SIZE(1);
    // Can't know index to expect

    // Check telemetry
    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_LOAD_COUNT_SIZE(1);
    // Can't know the LOAD_COUNT to expect
  }

  void Tester::expectSpacePostFileCorrectForMessage(constSpacePostFile &spacePostFile, const SpacePost &message)
  {
    EXPECT_EQ(spacePostFile.getMessageText().length(), message.getmessage_content().length())
        << "Unexpected message text length in loaded message";

    EXPECT_STREQ(spacePostFile.getMessageText().c_str(), message.getmessage_content().toChar())
        << "Unexpected message text content in loaded message";

    spacePostFile.expectIsValid();
  }

  void Tester::expectSpacePostTextEquals(const SpacePosts::SpacePost &message, const std::string &expected_content)
  {
    const SpacePosts::SpacePost::message_contentString &loaded_message_fprime_string = message.getmessage_content();

    // Check that message content is the same
    const std::string loaded_message_std_string_limitedLength{loaded_message_fprime_string.toChar(),
                                                              loaded_message_fprime_string.length()};
    EXPECT_EQ(loaded_message_std_string_limitedLength, expected_content)
        << "Unexpected message text content in loaded SpacePost";

    // Check that null terminator is present and correctly placed
    const std::string loaded_message_std_string_unlimitedLength{loaded_message_fprime_string.toChar()};
    EXPECT_EQ(loaded_message_std_string_unlimitedLength, expected_content)
        << "Null terminator not placed correctly in asserted SpacePost";
  }

  void Tester ::
      realizeDirectorySetupAndInitializeComponents()
  {
    // Clear events from any previous test in the rare case that a Tester object is used for multiple tests
    this->clearHistory();

    const U32 expected_index = this->m_directory.getNextSpacePostIndex();
    this->m_directory.realizeOnFileSystem();

    this->init();
    this->component.init(
        INSTANCE);

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_INDEX_RESTORE_COMPLETE_SIZE(1);
    if (this->m_directory.getExistingSpacePostIndices().empty())
    {
      ASSERT_EVENTS_INDEX_RESTORE_COMPLETE(0, this->m_directory.getExistingSpacePostIndices().size(), 0);
    }
    else
    {
      ASSERT_EVENTS_INDEX_RESTORE_COMPLETE(0, this->m_directory.getExistingSpacePostIndices().size(), expected_index - 1);
    }

    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX_SIZE(1);
    ASSERT_TLM_NEXT_STORAGE_INDEX(0, expected_index);

    this->clearHistory(); // Hide initialization events and telemetry from test methods
  }

  // ----------------------------------------------------------------------
  // F' Tester Implementations
  // ----------------------------------------------------------------------

  void Tester ::
      connectPorts()
  {

    // storeMessage
    this->connect_to_storeMessage(
        0,
        this->component.get_storeMessage_InputPort(0));

    // loadMessageFromIndex
    this->connect_to_loadMessageFromIndex(
        0,
        this->component.get_loadMessageFromIndex_InputPort(0));

    // loadMessageLastN
    this->connect_to_loadMessageLastN(
        0,
        this->component.get_loadMessageLastN_InputPort(0));

    // eventOut
    this->component.set_eventOut_OutputPort(
        0,
        this->get_from_eventOut(0));

    // textEventOut
    this->component.set_textEventOut_OutputPort(
        0,
        this->get_from_textEventOut(0));

    // timeGetOut
    this->component.set_timeGetOut_OutputPort(
        0,
        this->get_from_timeGetOut(0));

    // tlmOut
    this->component.set_tlmOut_OutputPort(
        0,
        this->get_from_tlmOut(0));
  }

} // end namespace SpacePosts
