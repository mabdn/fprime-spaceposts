#ifndef REF_MESSAGESTORAGE_TEST_UT_SPACEPOSTFILE_HPP
#define REF_MESSAGESTORAGE_TEST_UT_SPACEPOSTFILE_HPP

#include <string>
#include <ostream>

#include <Fw/Types/BasicTypes.hpp>

namespace SpacePosts
{
    /**
     * @brief Class that represents a file used by the MessageStorage component to store SpacePosts
     * in the storage directory.
     *
     * Consists of a delimiter, the length of the message text, and the message text itself.
     *
     * ASpacePostFile is considered to be valid iff it adheres to the component's storage format:
     * - The first byte is the delimiter byte (MESSAGESTORAGE_MSGFILE_DELIMITER)
     * - The second to fifth byte are the length of the message text as a 32-bit unsigned integer in little-endian byte
     *   order
     * - The rest of the bytes are the message text encoded as specified by Fw::String.serialize()
     *
     * This class uses white-box knowledge of the MessageStorage component as the storage format of the
     * MessageStorage component is internal to the component.
     * For testing the MessageStorage's behavior with valid and invalid SpacePost files in the storage directory,
     * it makes sense to leak this information into the unit tests.
     */
    classSpacePostFile
    {
    private:
        U8 m_delimiterMetaData;
        U32 m_messageLengthMetaData;
        std::string m_messageText;

        /**
         * When serializing a SpacePost's message_content field, the serialization process prepends the length of the
         * serialized string as a 16-bit unsigned integer in little-endian byte order in front of the message_content
         * string.
         *
         * Here, we use the white-box knowledge of how exactly the MessageStorage type is currently defined.
         * Note that the MessageStorage abstracts from this information by working on a Fw::Serializable instead.
         * However, to fully model a stored SpacePost file, we leak this information into the tests in order to have
         * full control of what files should contain and should not contain. Without knowing the exact type that is
         * serialized as the message_content field, we would not be able to do so.
         */
        U16 m_serializationLengthMetaData;

    public:
        /**
         * @brief Default constructor for an uninitializedSpacePostFile.
         *
         * TheSpacePostFile will most likely be invalid, but this is not guaranteed.
         */
       SpacePostFile() = default;

        /**
         * @brief Constructs aSpacePostFile representing a valid message file with random message text of random length.
         *
         * The meta data of theSpacePostFile is set so that it is valid generated text content.
         *
         * The length of the random message text is chosen randomly between 0 and MESSAGESTORAGE_MAX_MESSAGE_LENGTH.
         *
         * @param alphaNumericOnly  If true, the random message text will only contain alpha-numeric characters
         *                          (i.e., [a-zA-Z0-9])
         *                          If false, the random message text can contain any bytes except for the
         *                          null-terminator (i.e., [0x01-0xFF])
         */
       SpacePostFile(const bool alphaNumericOnly);

        /**
         * @brief Constructs aSpacePostFile representing a valid message file with random message text of the given length.
         *
         * The meta data of theSpacePostFile is set so that it is valid generated text content.
         *
         * @param textLength The length of the random message text to generate
         * @param alphaNumericOnly  If true, the random message text will only contain alpha-numeric characters
         *                          (i.e., [a-zA-Z0-9])
         *                          If false, the random message text can contain any bytes except for the
         *                          null-terminator (i.e., [0x01-0xFF])
         */
       SpacePostFile(const U32 textLength, const bool alphaNumericOnly);

        /**
         * @brief Constructs aSpacePostFile representing a message file with the given meta data and message text.
         *
         * Does not need to be valid file.
         *
         * @param delimiterMetaData The delimiter byte of theSpacePostFile
         * @param messageLengthMetaData The message length meta data of theSpacePostFile
         *                              It is the length of the serialized SpacePost (!= the length of the message text)
         * @param serializationLengthMetaData The meta data of the serialized representation of the message text.
         *                                    It is the length of the encoded string.
         * When serializing a SpacePost's message_content field, the serialization process prepends the length of the
         * serialized string as a 16-bit unsigned integer in little-endian byte order in front of the message_content
         * string.
         * @param messageText The message text of theSpacePostFile
         */
       SpacePostFile(const U8 delimiterMetaData, const U32 messageLengthMetaData,
                const U16 serializationLengthMetaData, const std::string messageText);

        // --------------------------------------------------------------
        // Methods for Comparing and Asserting
        // --------------------------------------------------------------

        /**
         * @brief Comparison operator forSpacePostFiles that compares all meta data and the message text of theSpacePostFiles.
         */
        bool operator==(constSpacePostFile &other) const;

        /**
         * @brief Get the Delimiter Meta Data byte of this file
         *
         * @return U8 the Delimiter Meta Data byte
         */
        U8 getDelimiterMetaData() const { return m_delimiterMetaData; }

        /**
         * @brief Get the Message Length Meta Data of this file
         *
         * @return U32 the Message Length Meta Data
         */
        U32 getMessageLengthMetaData() const { return m_messageLengthMetaData; }

        /**
         * @brief Get the Message Text of this file
         *
         * @return std::string the Message Text
         */
        std::string getMessageText() const { return m_messageText; }

        /**
         * @brief Get the Serialization Length Meta Data of this file
         *
         * @return U16 the Serialization Length Meta Data
         */
        U16 getSerializationLengthMetaData() const { return m_serializationLengthMetaData; }

        /**
         * @brief Asserts that thisSpacePostFile is valid.
         *
         * Refer to class description for definition of "valid."
         */
        void expectIsValid() const;

        // --------------------------------------------------------------
        // Methods for Writing and Reading to/from Storage Directory
        // --------------------------------------------------------------

        /**
         * @brief Writes theSpacePostFile to an actual file in the storage directory in the file system.
         *
         * Overwrites any existing file with the same name.
         *
         * @param index The index of theSpacePostFile in the storage directory. Determines the file name.
         *             E.g., if index is 5, the file will be named "5.spacepost".
         */
        void writeToStorageDirectory(const U32 index) const;

        /**
         * @brief Reads a message file stored on the actual file system (i.e., on disk) and sets thisSpacePostFile's
         * meta data and message text to the contents of the file. I.e., thisSpacePostFile will represent the contents
         * of the message file.
         *
         * @param index the index of the message file to read
         */
        void readFromStorageDirectory(const U32 index);
    };

    /**
     * @brief Overload of the << operator forSpacePostFiles to print them on the terminal.
     */
    std::ostream &operator<<(std::ostream &os, constSpacePostFile &spacePostFile);
}

#endif