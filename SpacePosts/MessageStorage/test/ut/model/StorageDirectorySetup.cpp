#include <filesystem>

#include "StorageDirectorySetup.hpp"
#include "Os/FileSystem.hpp"
#include "Os/Directory.hpp"
#include "Os/File.hpp"
#include "STest/Pick/Pick.hpp"
#include "STest/STest/testing.hpp"

namespace SpacePosts
{
    const std::string StorageDirectorySetup::STORAGE_DIRECTORY_PATH = MESSAGESTORAGE_MSGFILE_DIRECTORY;

    StorageDirectorySetup::StorageDirectorySetup(const bool exists) : m_directoryExists(exists)
    {
    }

    StorageDirectorySetup::StorageDirectorySetup(const U32 numberOfSpacePosts, const U32 indexOfFirstSpacePost,
                                                 std::function<U32()> indexStepGenerator,
                                                 std::vector<std::string> otherFilesNames)
        : m_directoryExists(true), m_otherFilesNames(otherFilesNames)
    {
        U32 nextIndex = indexOfFirstSpacePost;
        for (size_t i = 0; i < numberOfSpacePosts; i++)
        {
            m_spacePostFiles[nextIndex] =SpacePostFile{false};
            nextIndex += indexStepGenerator();
        }
    }

    std::vector<U32> StorageDirectorySetup::getExistingSpacePostIndices() const
    {
        std::vector<U32> keys;
        for (const auto &[key, _] : m_spacePostFiles)
        {
            keys.push_back(key);
        }

        return keys;
    }

    std::map<U32,SpacePostFile> StorageDirectorySetup::getLastNSpacePostFiles(const U32 num_files_to_get) const
    {
        std::map<U32,SpacePostFile> last_n_files;
        std::vector<U32> existingIndices = this->getExistingSpacePostIndices();
        std::sort(existingIndices.begin(), existingIndices.end());
        std::reverse(existingIndices.begin(), existingIndices.end());

        const U32 num_files = std::min(
            std::min(num_files_to_get, static_cast<U32>(existingIndices.size())),
            static_cast<U32>(SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size));

        for (size_t i = 0; i < num_files; i++)
        {
            last_n_files[existingIndices[i]] = m_spacePostFiles.at(existingIndices[i]);
        }

        return last_n_files;
    }

    void StorageDirectorySetup::setLastSpacePostFile(const std::vector<SpacePostFile> newLastSpacePostFiles)
    {
        std::vector<U32> existingIndices = this->getExistingSpacePostIndices();
        std::sort(existingIndices.begin(), existingIndices.end());
        auto new_spacePostfiles_iter = newLastSpacePostFiles.crbegin();

        // Overwrite the last N existing files
        const U32 num_messages_to_overwrite = std::min(existingIndices.size(), newLastSpacePostFiles.size());
        for (size_t i = existingIndices.size() - num_messages_to_overwrite; i < existingIndices.size(); i++)
        {
            this->m_spacePostFiles[existingIndices[i]] = *new_spacePostfiles_iter++;
        }

        // Add remaining files, if there are any, to the end
        for (size_t i = 0; i < newLastSpacePostFiles.size() - num_messages_to_overwrite; i++)
        {
            this->addSpacePostFile(this->getNextSpacePostIndex(), *new_spacePostfiles_iter++);
        }
    }

    U32 StorageDirectorySetup::getNextSpacePostIndex() const
    {
        std::vector<U32> existingIndices = this->getExistingSpacePostIndices();
        if (existingIndices.empty())
        {
            return MESSAGESTORAGE_INITIAL_INDEX;
        }

        return *std::max_element(existingIndices.cbegin(), existingIndices.cend()) + 1;
    }

    U32 StorageDirectorySetup::getRandomFreeIndex() const
    {
        std::vector<U32> existingIndices = this->getExistingSpacePostIndices();
        U32 randomIndex;
        do
        {
            randomIndex = STest::Pick::any();
        } while (std::find(existingIndices.cbegin(), existingIndices.cend(), randomIndex) != existingIndices.cend());

        return randomIndex;
    }

    void StorageDirectorySetup::addSpacePostFile(const U32 index, constSpacePostFile file)
    {
        this->m_spacePostFiles[index] = file;
    }

    void StorageDirectorySetup::realizeOnFileSystem() const
    {
        // Reset the directory on the file system
        this->deleteAllFilesOnFileSystem();
        Os::FileSystem::removeDirectory(STORAGE_DIRECTORY_PATH.c_str());

        // Done if the storage directory is not supposed to exist
        if (!this->m_directoryExists)
        {
            return;
        }

        Os::FileSystem::createDirectory(STORAGE_DIRECTORY_PATH.c_str());

        for (auto const &[index, spacePostFile] : m_spacePostFiles)
        {
            spacePostFile.writeToStorageDirectory(index);
        }

        for (const std::string &fileName : m_otherFilesNames)
        {
            Os::File file;
            file.open((STORAGE_DIRECTORY_PATH + fileName).c_str(), Os::File::OPEN_CREATE);
            file.close();
        }
    }

    void StorageDirectorySetup::expectAllSpacePostFilesAreOnDiskAndAreUnchanged() const
    {
        for (const auto &[index, setupSpacePostFile] : m_spacePostFiles)
        {
           SpacePostFile fileSystemSpacePostFile{};
            fileSystemSpacePostFile.readFromStorageDirectory(index);
            EXPECT_EQ(setupSpacePostFile, fileSystemSpacePostFile)
                << "SpacePostFile with index " << index << " was not found on the file system or was changed.";
        }
    }

    // ----------------------------------------------------------------------
    // Private Helper Methods
    // ----------------------------------------------------------------------

    void StorageDirectorySetup ::
        deleteAllFilesOnFileSystem()
    {

        std::filesystem::path directoryPath(MESSAGESTORAGE_MSGFILE_DIRECTORY);
        if (!std::filesystem::exists(directoryPath))
        {
            return;
        }

        for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
        {
            std::filesystem::remove(entry.path());
        }
    }
}