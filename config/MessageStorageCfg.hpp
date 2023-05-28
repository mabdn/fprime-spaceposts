/*
 * MessageStorageCfg.hpp
 *
 *  Created on: Apr 7, 2023
 *      Author: Marius Baden
 */

#ifndef MessageStorage_MessageStorageCfg_HPP_
#define MessageStorage_MessageStorageCfg_HPP_

#include <string>
#include <regex>

#include "SpacePosts/MessageTypes/FppConstantsAc.hpp"

// Anonymous namespace for configuration parameters
namespace
{

  enum
  { 
    // Initial index for SpacePost files to use if no files are found in the storage directory, and
    // thus indexing cannot be continued from the last found index.
    // 
    // E.g., will be used to store the first SpacePost.
    MESSAGESTORAGE_INITIAL_INDEX = 0,
    
    // Byte value that is placed + expected at the beginning of every
    // valid SpacePost file.
    // Basic sanity check against file integrity + parsing wrong files
    MESSAGESTORAGE_MSGFILE_DELIMITER = 0xD9,

    // The maximum number of indices of validly stored SpacePosts to keep in the lastSuccessfullyStoredIndices data
    // strucutre. 
    //
    // See the documentation of MessageStorage::lastSuccessfullyStoredIndices for more information.
    MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE = SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size,

    // Maximum number of characters for the name of a SpacePost file (incl. the extension)
    // (in char* representation of Os::Directory::read()).
    //
    // Count does not include a terminating null character.
    // 
    // Currently: Maximum length of index (U32) as decimal string + length of extension
    MESSAGESTORAGE_MSGFILE_NAME_MAXLENGTH = strlen("4294967295") + strlen(".spaceposts")
  };

  // File extension for SpacePost files.
  //  To be appended to every SpacePost file name.
  //  Should start with a dot.
  static const std::string MESSAGESTORAGE_MSGFILE_FILE_EXTENSION{".spaceposts"};

  // Regex for matching SpacePost file names.
  //
  // Used to check if a file in the storage directory is a SpacePost file.
  static const std::regex MESSAGESTORAGE_MSGFILE_FILE_NAME_REGEX{"[0-9]{1,10}\\.spaceposts"};

  // Absolute path to the directory where SpacePost files are stored.
  // Should end with a slash.
  //
  // Example: If MESSAGESTORAGE_MSGFILE_DIRECTORY="/home/ground/", then
  // messages will be stored as "/home/ground/0.spaceposts", "/home/ground/1.spaceposts", etc.
  //
  // TODO Change for wherever you are using the component
  static const std::string MESSAGESTORAGE_MSGFILE_DIRECTORY{"/home/spaceposts/"};
}

#endif /* MessageStorage_MessageStorageCfg_HPP_ */
