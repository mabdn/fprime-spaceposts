#include <tuple>
#include <functional>
#include <string>
#include <vector>

#include "Tester.hpp"
#include "gtest/gtest.h"
#include "STest/Random/Random.hpp"
#include "STest/Pick/Pick.hpp"

#include "model/SpacePostFile.hpp"
#include "model/StorageDirectorySetup.hpp"
#include "data/StorageStateProvider.hpp"

using namespace SpacePosts;

constexpr const U32 MAX_MSGTEXT_LENGTH = SpacePosts::FppConstant_SpacePost_MaxTextLength::SpacePost_MaxTextLength;

constexpr const U32 MAX_MSGBATCH_SIZE = SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size;

/*
    UT-STO-010
    Test the assigned index to stored messages based on different content of the storage directory
*/

TEST_P(StorageStateProviderDetailed, TestStoreIndexNominal)
{
    tester.testStoreIndex(); // The only parameter that changes is the state of the storage directory
}

TEST(StorageStateProviderDetailed, TestStoreIndexErrorDirDoesNotExist)
{
    StorageDirectorySetup setup{false};
    Tester tester{setup};
    tester.testStoreIndexDirDoesNotExist();
}

/*
    UT-STO-020
    Test storing a message in an empty storage directory based the message’s text content
*/

TEST_P(StorageStateProviderCompact, TestStoreMessageTextNominalEmpty)
{
    tester.testStoreMessageText("");
}
TEST_P(StorageStateProviderCompact, TestStoreMessageTextNominalSingleChar)
{
    tester.testStoreMessageText("x");
}
TEST_P(StorageStateProviderCompact, TestStoreMessageTextNominalTypicalMessageSizeNormalChars)
{
    tester.testStoreMessageText(STest::Pick::stringAlphaNumeric(STest::Pick::lowerUpper(2, MAX_MSGTEXT_LENGTH - 1)));
}
TEST_P(StorageStateProviderCompact, TestStoreMessageTextNominalTypicalMessageSizeAllChars)
{
    tester.testStoreMessageText(STest::Pick::stringNonNull(STest::Pick::lowerUpper(2, MAX_MSGTEXT_LENGTH - 1)));
}
TEST_P(StorageStateProviderCompact, TestStoreMessageTextNominalMaxMessageSize)
{
    tester.testStoreMessageText(STest::Pick::stringNonNull(MAX_MSGTEXT_LENGTH));
}

/*
    UT-STO-030
    Test loading a message from a given index based on whether that index exists
*/

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalIndexExists)
{
    // Thanks to the various directorySetups that we are testing with, all kinds of possible indices are tested
    // In particular, every index in the data-driven test variable 'valid_index_of_first_spacePost_message' is tested
    U32 num_messages_loaded = 0;
    for (const U32 &index : directorySetup.getExistingSpacePostIndices())
    {
        tester.testLoadFromExistingIndex(index, num_messages_loaded);
        num_messages_loaded++;
    }
}

TEST_P(StorageStateProviderDetailed, TestLoadFromIndexFailIndexDoesNotExist)
{
    tester.testLoadFromNonExistingIndex(directorySetup.getRandomFreeIndex());
}

/*
    UT-STO-040
    Test loading a message from a given index based on the validity of the file on disk referenced by the index

    The index is picked arbitrarily for these tests
*/

// --- Tests with valid files ---

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalEmptyString)
{
    tester.testLoadValidSpacePostFileFromIndex(0xFD43,SpacePostFile{0, true});
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalSingleChar)
{
    tester.testLoadValidSpacePostFileFromIndex(std::numeric_limits<U32>::max(),SpacePostFile{1, false});
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalTypicalMessageSizeNormalChars)
{
    tester.testLoadValidSpacePostFileFromIndex(0,SpacePostFile{STest::Pick::lowerUpper(2, MAX_MSGTEXT_LENGTH - 1), true});
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalTypicalMessageSizeAllChars)
{
    tester.testLoadValidSpacePostFileFromIndex(1,SpacePostFile{STest::Pick::lowerUpper(2, MAX_MSGTEXT_LENGTH - 1), false});
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexNominalMaxMessageSize)
{
    tester.testLoadValidSpacePostFileFromIndex(42,SpacePostFile{MAX_MSGTEXT_LENGTH, false});
}

// --- Tests with invalid files ---
//
// They use a litte piece of white box knowledge to know the expected error code.
//
// Ideally, would want to succeed test with matching any
// error code as the error code is only used for logging purposes and not part of the actual functionality.
// However, the F' framework does not allow matching any error code while still checking for the MessageReadError
// type.

// Message does fit into the allocated buffer with its full text, but the message text is not null terminated
// => Should fail while deserilizing as the message is malformed
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorMaxMessageSizePlusOne)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),SpacePostFile{MAX_MSGTEXT_LENGTH + 1, false},
                                           MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
                                           Fw::FW_DESERIALIZE_SIZE_MISMATCH);
}

// Message does not fit into the allocated buffer even without its null terminator
// => Should fail check of the encoded message size
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorMaxMessageSizePlusTwo)
{
    const U32 length_too_long = MAX_MSGTEXT_LENGTH + 2;
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{length_too_long, false},
                                           MessageReadError::MESSAGE_SIZE_EXCEEDS_BUFFER, length_too_long + 2);
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorMessageWayTooLong)
{
    const U32 length_way_too_long = STest::Pick::lowerUpper(MAX_MSGTEXT_LENGTH + 3, 1000);
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{length_way_too_long, false},
                                           MessageReadError::MESSAGE_SIZE_EXCEEDS_BUFFER, length_way_too_long + 2);
}

// Test meta data with a fixed arbitrary message content 'Hello World' (length 11)
// This makes use of the *Independence Assumption* as outlined in the unit test documentation for the MessageStorage
// component.

TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidDelimiter)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),SpacePostFile{0xD8, 13, 11, "Hello World"},
                                           MessageReadError::DELIMITER_CONTENT, 0xD8);
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthMetaData0)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 0, 11, "Hello World"},
                                           MessageReadError::MESSAGE_SIZE_ZERO,
                                           0);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthMetaDataWayTooShort)
{
    const U32 message_length_too_short = STest::Pick::lowerUpper(1, 13 - 2);
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, message_length_too_short, 11, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
                                           Fw::FW_DESERIALIZE_SIZE_MISMATCH);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthMetaDataOneTooShort)
{
    const U32 message_length_too_short = 13 - 1;
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, message_length_too_short, 11, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
                                           Fw::FW_DESERIALIZE_SIZE_MISMATCH);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthMetaDataOneTooLong)
{
    const U32 message_length_too_long = 13 + 1;
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, message_length_too_long, 11, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_SIZE,
                                           0);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthMetaDataWayTooLong)
{
    const U32 message_length_too_long = STest::Pick::lowerUpper(13 + 2, MAX_MSGTEXT_LENGTH);
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, message_length_too_long, 11, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_SIZE,
                                           0);
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidSerializationLengthMetaDataZero)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13, 0, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH,
                                           11);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidSerializationLengthMetaDataWayTooShort)
{
    const U16 serialization_length_too_short = STest::Pick::lowerUpper(0, 11 - 2);
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13, serialization_length_too_short, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH,
                                           11 - serialization_length_too_short);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidSerializationLengthMetaDataOneTooShort)
{
    const U16 serialization_length_too_short = 11 - 1;
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13, serialization_length_too_short, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_READ_LENGTH,
                                           1);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidSerializationLengthMetaDataOneTooLong)
{
    const U16 serialization_length_too_long = 11 + 1;
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13, serialization_length_too_long, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
                                           Fw::FW_DESERIALIZE_SIZE_MISMATCH);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidSerializationLengthMetaDataWayTooLong)
{
    const U16 serialization_length_too_long = STest::Pick::lowerUpper(11 + 2, 1000);
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13, serialization_length_too_long, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_DESER_EXCECUTE,
                                           Fw::FW_DESERIALIZE_SIZE_MISMATCH);
}

TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthAndSerializationLengthTooShortButConsistent)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13 - 1, 11 - 1, "Hello World"},
                                           MessageReadError::FILE_END,
                                           Os::File::OP_OK);
}
TEST_P(StorageStateProviderCompact, TestLoadFromIndexErrorInvalidMessageLengthAndSerializationLengthTooLongButConsistent)
{
    tester.testLoadInvalidSpacePostFileFromIndex(directorySetup.getRandomFreeIndex(),
                                          SpacePostFile{0xD9, 13 + 1, 11 + 1, "Hello World"},
                                           MessageReadError::MESSAGE_CONTENT_SIZE,
                                           0);
}

/*
    UT-STO-050
    Test whether loading last N messages selects the most recently stored messages based on different numbers for N

    Thanks to the various directorySetups that we are testing with, these tests are executed with all kinds of different
    numbers of message files existing in the directory. Thus, we only need to define a test per different number of
    messages to load
*/

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalNone)
{
    tester.testLoadLastNMessagesExistingInDirectory(0);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalOne)
{
    tester.testLoadLastNMessagesExistingInDirectory(1);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalDirSizeMinusOne)
{
    const U32 max_minus_one = directorySetup.getExistingSpacePostIndices().size() == 0
                                  ? 0
                                  : directorySetup.getExistingSpacePostIndices().size() - 1;
    tester.testLoadLastNMessagesExistingInDirectory(max_minus_one);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalDirSize)
{
    tester.testLoadLastNMessagesExistingInDirectory(directorySetup.getExistingSpacePostIndices().size());
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalDirSizePlusOne)
{
    tester.testLoadLastNMessagesExistingInDirectory(directorySetup.getExistingSpacePostIndices().size() + 1);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalBatchSizeMinusOne)
{
    tester.testLoadLastNMessagesExistingInDirectory(MAX_MSGBATCH_SIZE - 1);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalBatchSize)
{
    tester.testLoadLastNMessagesExistingInDirectory(MAX_MSGBATCH_SIZE);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalBatchSizePlusOne)
{
    tester.testLoadLastNMessagesExistingInDirectory(MAX_MSGBATCH_SIZE + 1);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalWayTooLong)
{
    tester.testLoadLastNMessagesExistingInDirectory(STest::Pick::lowerUpper(
        MAX_MSGBATCH_SIZE + 2, std::numeric_limits<U8>::max()));
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalDomainMaxMinusOne)
{
    tester.testLoadLastNMessagesExistingInDirectory(std::numeric_limits<U8>::max() - 1);
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectIndicesNominalDomainMax)
{
    tester.testLoadLastNMessagesExistingInDirectory(std::numeric_limits<U8>::max());
}
// Domain max + 1 not possible: Port parameter type (U8) cannot hold more than its maximum value

/*
    UT-STO-060
    Test loading the last N messages based on the validity of the corresponding message files on disk

    Since loading the last N messages uses the same code as loading a single message from a given index, we do not
    test the cartesian product of all possible file contents for every message in the set of last N messages.
*/

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectMessageContentNominalOneValid)
{
    constSpacePostFile A{0xD9, 30, 28, "My test message A! (Valid)  "};

    tester.testLoadLastNMessagesGivenSpacePostFiles(
        1,
        std::vector<SpacePostFile>{A},
        std::vector<SpacePostFile>{A});
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectMessageContentNominalThreeValid)
{
    constSpacePostFile A{0xD9, 30, 28, STest::Pick::stringNonNull(28)}; // valid
    constSpacePostFile B{0xD9, 30, 28, STest::Pick::stringNonNull(28)}; // valid
    constSpacePostFile C{0xD9, 30, 28, STest::Pick::stringNonNull(28)}; // valid

    tester.testLoadLastNMessagesGivenSpacePostFiles(
        3,
        std::vector<SpacePostFile>{A, B, C},
        std::vector<SpacePostFile>{A, B, C});
}

// Invalid message files should be skipped in loading. Thus we expect only the valid message to be loaded
TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectMessageContentNominalOneInvalid)
{
    constSpacePostFile A{0xD8, 30, 28, "My test message A! (Invalid)"};
    constSpacePostFile B{0xD9, 30, 28, "My test message B! (Valid)  "};

    tester.testLoadLastNMessagesGivenSpacePostFiles(
        1,
        std::vector<SpacePostFile>{A, B},
        std::vector<SpacePostFile>{B});
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectMessageContentNominalMoreValidThanInvalid)
{
    constSpacePostFile A{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid
    constSpacePostFile B{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid
    constSpacePostFile C{0xD9, 30, 28, STest::Pick::stringNonNull(MAX_MSGTEXT_LENGTH + 1)}; // invalid
    constSpacePostFile D{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid

    tester.testLoadLastNMessagesGivenSpacePostFiles(
        3,
        std::vector<SpacePostFile>{A, B, C, D},
        std::vector<SpacePostFile>{A, B, D});
}

TEST_P(StorageStateProviderCompact, TestLoadLastNLoadsCorrectMessageContentNominalMoreInvalidThanValid)
{
    constSpacePostFile A{0xD9, 31, 29, STest::Pick::stringNonNull(28)};                     // invalid
    constSpacePostFile B{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid
    constSpacePostFile C{0xD9, 30, 27, STest::Pick::stringNonNull(28)};                     // invalid
    constSpacePostFile D{0xD9, 30, 28, STest::Pick::stringNonNull(MAX_MSGTEXT_LENGTH + 1)}; // invalid
    constSpacePostFile E{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid
    constSpacePostFile F{0xD9, 31, 28, STest::Pick::stringNonNull(28)};                     // invalid
    constSpacePostFile G{0xD9, 30, 28, STest::Pick::stringNonNull(28)};                     // valid

    tester.testLoadLastNMessagesGivenSpacePostFiles(
        3,
        std::vector<SpacePostFile>{A, B, C, D, E, F, G},
        std::vector<SpacePostFile>{B, E, G});
}

/*

    ---- White-Box Tests ----

*/

/*
    UT-STO-110
    Test fail but no crash if no new message file can be created when trying to store a message

    The only parameter that changes is the state of the storage directory
    (provided by the StorageStateProviderDetailed).
    The index to store the message varies with the different states of the storage directory.
*/

TEST_P(StorageStateProviderDetailed, TestStoreErrorFileCreateFails)
{
    this->tester.testStoreFileCreateFails();
}

/*
    UT-STO-120
    Test fail but no crash if message file already exists for index used to store a message

    The storage directory state is the only parameter: Same as for UT-STO-110.
*/

TEST_P(StorageStateProviderDetailed, TestStoreErrorFileExists)
{

    this->tester.testStoreFileExists();
}

/*
    Instantiate and Execute
*/

// Instantiate detailed group
INSTANTIATE_TEST_SUITE_P(DetailedStorageDirectorySetupSet,
                         StorageStateProviderDetailed,
                         directorySetupParameterDetailed);

// Instantiate compact group
INSTANTIATE_TEST_SUITE_P(CompactStorageDirectorySetupSet,
                         StorageStateProviderCompact,
                         directorySetupParameterCompact);

// Execute tests
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}