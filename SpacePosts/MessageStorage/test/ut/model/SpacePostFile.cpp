#include <fstream>

#include "Fw/Types/SerialBuffer.hpp"
#include "Fw/Types/String.hpp"
#include "SpacePosts/MessageTypes/SpacePostSerializableAc.hpp"
#include "STest/Pick/Pick.hpp"
#include "Fw/Types/Assert.hpp"
#include "gtest/gtest.h"

#include "SpacePosts/MessageTypes/FppConstantsAc.hpp"
#include "config/MessageStorageCfg.hpp"
#include "SpacePostFile.hpp"

SpacePosts::SpacePostFile::SpacePostFile(const bool alphaNumericOnly)
    :SpacePostFile(STest::Pick::lowerUpper(0, SpacePosts::FppConstant_SpacePost_MaxTextLength::SpacePost_MaxTextLength),
              alphaNumericOnly)
{
}

SpacePosts::SpacePostFile::SpacePostFile(const U32 textLength, const bool alphaNumericOnly)
{
    if (alphaNumericOnly)
    {
        m_messageText = STest::Pick::stringAlphaNumeric(textLength);
    }
    else
    {
        m_messageText = STest::Pick::stringNonNull(textLength);
    }
    FW_ASSERT(m_messageText.size() == textLength, m_messageText.size(), textLength);

    // Set meta data
    m_delimiterMetaData = MESSAGESTORAGE_MSGFILE_DELIMITER;
    m_serializationLengthMetaData = m_messageText.size();
    m_messageLengthMetaData = m_serializationLengthMetaData + 2; // Fw::String.serialize() adds 2 bytes for its own size meta data
}

SpacePosts::SpacePostFile::SpacePostFile(const U8 delimiterMetaData, const U32 messageLengthMetaData,
                      const U16 serializationLengthMetaData, const std::string messageText)
    : m_delimiterMetaData(delimiterMetaData), m_messageLengthMetaData(messageLengthMetaData),
      m_serializationLengthMetaData(serializationLengthMetaData), m_messageText(messageText)
{
}

bool SpacePosts::SpacePostFile::operator==(constSpacePostFile &other) const
{
    return (this->m_delimiterMetaData == other.m_delimiterMetaData) &&
           (this->m_messageLengthMetaData == other.m_messageLengthMetaData) &&
           (this->m_messageText == other.m_messageText) &&
           (this->m_serializationLengthMetaData == other.m_serializationLengthMetaData);
}

void SpacePosts::SpacePostFile::expectIsValid() const
{
    EXPECT_EQ(m_delimiterMetaData, MESSAGESTORAGE_MSGFILE_DELIMITER)
        << "SpacePostFile is invalid: Delimiter Meta Data is not correct";
    EXPECT_EQ(m_messageLengthMetaData, m_messageText.size() + 2)
        << "SpacePostFile is invalid: Message Length Meta Data is not correct";
    EXPECT_EQ(m_serializationLengthMetaData, m_messageText.size())
        << "SpacePostFile is invalid: Serialization Length Meta Data is not correct";
}

void SpacePosts::SpacePostFile::writeToStorageDirectory(const U32 index) const
{
    // Open file
    std::ofstream file;
    file.open((MESSAGESTORAGE_MSGFILE_DIRECTORY + std::to_string(index) + MESSAGESTORAGE_MSGFILE_FILE_EXTENSION).c_str(),
              std::ios::out | std::ios::binary);

    // Write meta data
    file.write(reinterpret_cast<const char *>(&m_delimiterMetaData), sizeof(m_delimiterMetaData));

    // Write message length meta data in little-endian order
    file.put(m_messageLengthMetaData >> 24);
    file.put(m_messageLengthMetaData >> 16);
    file.put(m_messageLengthMetaData >> 8);
    file.put(m_messageLengthMetaData);

    // Write Fw::String.serialize()'s size meta data in little-endian order
    file.put(m_serializationLengthMetaData >> 8);
    file.put(m_serializationLengthMetaData);

    // Write message text
    file.write(m_messageText.c_str(), m_messageText.size());

    // Close file
    file.close();
}

void SpacePosts::SpacePostFile::readFromStorageDirectory(const U32 index)
{
    // Open file
    std::ifstream file;
    file.open((MESSAGESTORAGE_MSGFILE_DIRECTORY + std::to_string(index) + MESSAGESTORAGE_MSGFILE_FILE_EXTENSION).c_str(),
              std::ios::in | std::ios::binary);

    // Read meta data
    file.read(reinterpret_cast<char *>(&m_delimiterMetaData), sizeof(m_delimiterMetaData));

    // Read message length meta data in little-endian order
    this->m_messageLengthMetaData = 0;
    this->m_messageLengthMetaData |= file.get() << 24;
    this->m_messageLengthMetaData |= file.get() << 16;
    this->m_messageLengthMetaData |= file.get() << 8;
    this->m_messageLengthMetaData |= file.get();

    // Read Fw::String.serialize()'s size meta data in little-endian order
    // Fw::String.serialize() adds 2 bytes for its own size meta data
    this->m_serializationLengthMetaData = 0;
    this->m_serializationLengthMetaData |= file.get() << 8;
    this->m_serializationLengthMetaData |= file.get();

    // Read all remaining bytes as message text
    this->m_messageText = "";
    while (file.peek() != EOF)
    {
        this->m_messageText += file.get();
    }

    // Close file
    file.close();
}

std::ostream &SpacePosts::operator<<(std::ostream &os, constSpacePostFile &spacePostFile)
{
    return os << "SpacePostFile("
              << std::hex << std::showbase << spacePostFile.getDelimiterMetaData() << ", " 
              << std::dec << std::noshowbase  
              << spacePostFile.getMessageLengthMetaData() << ", " 
              << spacePostFile.getSerializationLengthMetaData() << ", " 
              << "\"" << spacePostFile.getMessageText() << "\")";
}
