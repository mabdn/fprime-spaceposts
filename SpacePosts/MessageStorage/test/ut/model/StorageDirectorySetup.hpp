#ifndef REF_MESSAGESTORAGE_TEST_UT_STORAGEDIRECTORYSETUP_HPP
#define REF_MESSAGESTORAGE_TEST_UT_STORAGEDIRECTORYSETUP_HPP

#include <functional>
#include <string>

#include "Fw/Types/BasicTypes.hpp"

#include "config/MessageStorageCfg.hpp"
#include "SpacePostFile.hpp"

namespace SpacePosts
{
    /**
     * @brief Class that represent a state of the storage directory when testing the MessageStorage component.
     *
     * By conducting each unit test under various storage directory setups, we can test the MessageStorage
     * component's behavior under various conditions similar to the ones it will encounter in flight.
     *
     * Empty files can be added with a given name,SpacePostFiles can be added with a given name and contents.
     *
     * Calling realizeOnFileSystem() will make the storage directory on disk (i.e., on the file system) match the state
     * described by this object.
     */
    class StorageDirectorySetup
    {
    private:
        static const std::string STORAGE_DIRECTORY_PATH;

        const bool m_directoryExists;

        //! TheSpacePostFiles that are supposed to exist in the storage directory and the index at which they are supposed to
        //! be stored.
        std::map<U32,SpacePostFile> m_spacePostFiles;

        std::vector<std::string> m_otherFilesNames;

    public:
        /**
         * @brief Construct a new Storage Directory Setup object which represents an empty storage directory.
         *
         * @param exists If false, the storage directory is not supposed to exist on the file system.
         *               It cannot be empty in that case. Upon calling realizeOnFileSystem(), the storage directory
         *               will be deleted if it does already exist on the file system.
         */
        StorageDirectorySetup(const bool exists = true);

        /**
         * @brief Construct a new Storage Directory Setup object which represents a filled storage directory.
         *
         * @param numberOfSpacePosts  The number of valid SpacePost files to create with random text content
         * @param indexOfFirstSpacePost The index of the first SpacePost file to create
         * @param indexStepGenerator A function that defines the step between SpacePost file indices.
         *                           Called once for each SpacePost file to define the index of the next message
         * @param otherFilesNames A list of files names. For each name in the list, an empty file will be created in the
         *                       storage directory
         */
        StorageDirectorySetup(const U32 numberOfSpacePosts, const U32 indexOfFirstSpacePost,
                              std::function<U32()> indexStepGenerator, std::vector<std::string> otherFilesNames);

        /**
         * @brief Get a vector of all indices of SpacePost files which exist in the storage directory.
         *
         * @return std::vector<U32> all indices of SpacePost files which exist in the storage directory
         */
        std::vector<U32> getExistingSpacePostIndices() const;

        /**
         * @brief Get the nSpacePostFiles with the highest indices which exist in the storage directory.
         * 
         * Get allSpacePostFiles if there are less than nSpacePostFiles in the storage directory.
         * 
         * Get a maximum of SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size manySpacePostFiles.
         * 
         * This interface definition mimics the behavior of the MessageStorage component's 
         * `loadMessageLastN` port.
         * 
         * @param numFilesToGet n: the number ofSpacePostFiles to get
         * @return std::vector<U32,SpacePostFile> theSpacePostFiles
         */
        std::map<U32,SpacePostFile> getLastNSpacePostFiles(const U32 numFilesToGet) const;

        /**
         * @brief Sets theSpacePostFiles with the highest indices in the storage directory to the givenSpacePostFiles.
         * 
         * If there are lessSpacePostFiles in the storage directory than the size of the given vector, the method adds the 
         * remainingSpacePostFiles to the storage directory at the next available index.
         * 
         * The firstSpacePostFile in the vector will be stored at the highest index in the storage directory.
         * 
         * @param newLastSpacePostFiles TheSpacePostFiles to set as the lastSpacePostFiles in the storage directory
         */
        void setLastSpacePostFile(const std::vector<SpacePostFile> newLastSpacePostFiles);

        /**
         * @brief Gets the highest used index for a SpacePost file in the directory + 1.
         *
         * @return U32 The highest used index for a SpacePost file in the directory + 1
         */
        U32 getNextSpacePostIndex() const;

        /**
         * @brief Randomly picks an index for a SpacePost which does not exist in the storage directory
         *
         * @return U32 an index for which no BSSMessage is stored in the storage directory
         */
        U32 getRandomFreeIndex() const;

        /**
         * @brief Add aSpacePostFile to the storage directory by storing its content in a file name with the given index.
         *
         * @param index The index of theSpacePostFile which will be used for the file name
         * @param file TheSpacePostFile to add
         */
        void addSpacePostFile(const U32 index, constSpacePostFile file);

        /**
         * @brief Makes the storage directory on disk (i.e., on the file system) match the state described by this
         *          object.
         */
        void realizeOnFileSystem() const;

        /**
         * @brief Expects (with gtest EXPECT_*) that allSpacePostFiles that are defined in this storage directory setup
         *        exist in the storage directory on the file system.
         *
         * Furthermore, the content of theSpacePostFiles on disk is expected to be the same as the content of theSpacePostFiles
         * defined in this storage directory setup.
         *
         */
        void expectAllSpacePostFilesAreOnDiskAndAreUnchanged() const;

    private:
        /**
         * @brief Deletes all files in the storage directory on disk (i.e., on the file system).
         */
        static void deleteAllFilesOnFileSystem();
    };
}

#endif