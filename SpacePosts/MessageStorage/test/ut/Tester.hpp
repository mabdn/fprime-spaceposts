// ======================================================================
// \title  MessageStorage/test/ut/Tester.hpp
// \author marius
// \brief  hpp file for MessageStorage test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef TESTER_HPP
#define TESTER_HPP

#define TEST_BUFFER_MAX_SIZE 100
#define BUFFER_MEMORY_DEFAULT_VALUE 0x42

#include "GTestBase.hpp"
#include "SpacePosts/MessageStorage/MessageStorage.hpp"
#include "model/StorageDirectorySetup.hpp"
#include "model/SpacePostFile.hpp"

namespace SpacePosts
{

  class Tester : public MessageStorageGTestBase
  {

  private:
    /**
     * Data-driven test data provider for different storage directory setups.
     */
    StorageDirectorySetup m_directory;

    /**
     * The component under test.
     */
    MessageStorage component;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    /**
     * @brief Construct a new Tester object which will test the MessageStorage component with
     *        the storage directory's state as specified by the given StorageDirectorySetup.
     *
     * By conducting each unit test under various storage directory setups, we can test the MessageStorage
     * component's behavior under various conditions similar to the ones it will encounter in flight.
     *
     * @param directorySetup the setup of the storage directory for this test
     * (see StorageDirectorySetup.hpp)
     */
    Tester(const StorageDirectorySetup directorySetup);

    /**
     * @brief Destroy the Tester object
     */
    ~Tester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    /*
      UT-STO-010
      Test the assigned index to stored messages based on different content of the storage directory
    */

    /**
     * @brief Lets the component store a message and checks whether the message was stored at the correct index.
     *
     * The correct index is the next available index in the storage directory. It depends on the
     * StorageDirectorySetup.
     */
    void testStoreIndex();

    /**
     * @brief Tests whether the component handles a situation with a non-existing storage directory correctly.
     *
     * The component is expected to create the storage directory. Next, it should store the message at index
     * MESSAGESTORAGE_INITIAL_INDEX since no other messages have been stored yet.
     *
     * Expects the storage directory setup to be configured so that the storage directory does not exist.
     *
     * A special case of testStoreIndex().
     */
    void testStoreIndexDirDoesNotExist();

    /*
      UT-STO-020
      Test storing a message in an empty storage directory based the message’s text content
    */

    /**
     * @brief Lets the component store a message with the given text and checks whether the message was stored
     *        in a correctly formated message file (as specified by the component design documentation)
     *        at the next available index.
     *
     * @param messageText The text of the SpacePost to store
     */
    void testStoreMessageText(const std::string messageText);

    /*
        UT-STO-030
        Test loading a message from a given index based on whether that index exists
    */

    /**
     * @brief Lets the component load a message form the given index and checks whether the message was loaded
     *        correctly.
     *
     * We only consider loading valid messages from valid indices in this test case.
     *
     * @param index The index of a valid message in the storage directory
     * @param num_message_loaded The number of messages that have been loaded from the storage directory before this test.
     *                           Use this parameter to check whether multiple loads in a row work correctly.
     */
    void testLoadFromExistingIndex(const U32 index, const U32 num_message_loaded);

    /**
     * @brief Lets the component load the message form the given non-existing index, and checks whether
     *        the component handles this failure as expected.
     *
     * @param index The index of the message to load. No BSS message with this index must exist in the storage directory.
     */
    void testLoadFromNonExistingIndex(const U32 index);

    /*
        UT-STO-040
        Test loading a message from a given index based on the validity of the file on disk referenced by the index
    */

    /**
     * @brief Stores the given validSpacePostFile in the storage directory at the given index, lets the component load the
     *        message via the loadMessageFromIndex port and checks whether the message was loaded correctly.
     *
     * @param index The index at which to store the message (overwrites existing message if necessary)
     *              in the directory and from which the component should then load the message
     * @param spacePostFile TheSpacePostFile which specifies what content to write to the file which will be loaded by the
     *                component. Thus, it specifies what the component will find in the file when trying to load the
     *                message in this test.
     *                The specified file must adhere to the component's file format for SpacePost files.
     */
    void testLoadValidSpacePostFileFromIndex(const U32 index, constSpacePostFile spacePostFile);

    /**
     * @brief Stores the given invalidSpacePostFile in the storage directory at the given index, lets the component load the
     *  message via the loadMessageFromIndex port and checks whether the component handles loading the invalid message
     *  as specified.
     *
     * Check whether the component triggers an error event indicating failure at the given stage
     * and with the given error code.
     *
     * Check whether, after failing to load the message, the component remains functional
     * and is ready to load any other messages.
     *
     *
     * @param index The index at which to store the message (overwrites existing message if necessary)
     *             in the directory and from which the component should then load the message
     * @param spacePostFile TheSpacePostFile which specifies what content to write to the file which will be loaded by the
     *                component. Thus, it specifies what the component will find in the file when trying to load the
     *                message in this test.
     *                Must define an invalid SpacePost file which is supposed to cause a failure in loading in
     *                the given error stage with the given error code.
     * @param expectedErrorStage The stage at which the component is expected to trigger an error event
     * @param expectedErrorCode The error code with which the component is expected to trigger an error event
     *
     */
    void testLoadInvalidSpacePostFileFromIndex(const U32 index, constSpacePostFile spacePostFile,
                                         const MessageReadError expectedErrorStage, const I32 expectedErrorCode);

    /*
        UT-STO-050
        Test whether loading last N messages selects the most recently stored messages based on different numbers for N
    */

    /**
     * @brief Lets the component load the last N messages from the storage directory and checks whether the returned
     *        messages are the ones the component's interface specification claims to return.
     *
     * The component must return the last N messages that were stored in the directory. If less than N
     * messages were stored, the component must return all messages that were stored.
     *
     * The test will test the component with theSpacePostFiles that are already stored in the storage directory.
     * To place certain consciously formattedSpacePostFiles in the storage directory,
     * use testLoadLastNMessagesGivenSpacePostFiles().
     *
     * @param numMessagesToLoad The number of messages to tell the component to load
     */
    void testLoadLastNMessagesExistingInDirectory(const U32 numMessagesToLoad);

    /*
        UT-STO-060
        Test loading the last N messages based on the validity of the corresponding message files on disk
    */

    /**
     * @brief Stores the givenSpacePostFiles as the most recent messages in the given order (last message arrives last) in
     *        the storage directory, lets the component load the last N messages from the storage directory,
     *        and checks whether the returned messages are the ones provided as expectedSpacePostFiles.
     *
     * Being able to passSpacePostFiles as the most recently stored messages allows to test whether the component loads
     * the last N messages correctly based on the file content for the last stored messages.
     *
     * Both vectors ofSpacePostFiles must be given with the last stored message first in the vector.
     *
     * @param numMessagesToLoad The number of messages to tell the component to load
     * @param lastSpacePostFilesInStorage TheSpacePostFiles which specify what content will be in the most recently
     *                 stored message files when the component is told to load the last N messages.
     *                 Has to be contain more or equally many messages than/as numMessagesToLoad.
     * @param spacePostsExpectedToLoad The SpacePosts which are expected to be returned by the component when
     *                                  loading the last N messages.
     *                                  Has to be contain less or equally many messages than/as numMessagesToLoad.
     */
    void testLoadLastNMessagesGivenSpacePostFiles(const U8 numMessagesToLoad,
                                            const std::vector<SpacePostFile> lastSpacePostFilesInStorage,
                                            const std::vector<SpacePostFile> spacePostFilesExpectedToLoad);

    /*
        U-STO-110
        Test fail but no crash if no new message file can be created when trying to store a message
    */

    void testStoreFileCreateFails();

    /*
        U-STO-120
        Test fail but no crash if message file already exists for index used to store a message
    */

    /**
     * @brief Places a file at the next index used for storing, lets the component store a message, and checks whether
     * the component handles the situation with the existing file correctly.
     *
     * The component is expected to abort the storing process and trigger an error event. The existing message
     * is not changed.
     *
     * Uses the next available index in the storage directory to test the collison of storage operation and existing
     * file.
     */
    void testStoreFileExists();

  private:
    // ----------------------------------------------------------------------
    // Helper Methods
    // ----------------------------------------------------------------------

    /**
     * @brief A basic test which checks whether the component has not crashed and is able to perform
     *        one storage operation followed by one loading operation.
     *
     * Tests whether storing a random message at the next free index is successful.
     * Then, tests whether the loadMessageLastN port returns the message that was just stored when calling it with N=1.
     */
    void testComponentFunctional();

    /**
     * @brief Assert that aSpacePostFile is exactly the file which the component is expected to write to the storage
     * when storing the given message.
     *
     * For this assetions to be successful, theSpacePostFile must contain exactly the same text content as the given message.
     * Additionally, all meta data needs to be valid and match the given message.
     *
     * @param spacePostFile  TheSpacePostFile which is expected to be written to the storage when storing the given message
     * @param text_message The message which is expected to be represented by theSpacePostFile
     */
    void expectSpacePostFileCorrectForMessage(constSpacePostFile &spacePostFile, const SpacePost &message);

    /**
     * @brief F' generated method for connecting the Tester to the component's ports.
     * 
     */
    void connectPorts();

    /**
     * @brief Realize the StorageDirectorySetup to the file system and initialize the MessageStorage component
     *  under test.
     *
     * Asserts that the component initializes correctly. Therefore, it needs to emit a
     * a INDEX_RESTORE_COMPLETE event and report the restored index via the NEXT_STORAGE_INDEX telemetry.
     */
    void realizeDirectorySetupAndInitializeComponents();
  };

} // end namespace SpacePosts

#endif
