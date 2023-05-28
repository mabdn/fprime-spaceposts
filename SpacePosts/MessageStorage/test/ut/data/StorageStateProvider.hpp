#ifndef REF_MESSAGESTORAGE_TEST_UT_DATA_STORAGEDIRECTORYSTATES_HPP
#define REF_MESSAGESTORAGE_TEST_UT_DATA_STORAGEDIRECTORYSTATES_HPP

#include <tuple>
#include <functional>
#include <string>
#include <vector>

#include "Tester.hpp"
#include "gtest/gtest.h"
#include "STest/Random/Random.hpp"
#include "STest/Pick/Pick.hpp"

#include "../model/SpacePostFile.hpp"
#include "../model/StorageDirectorySetup.hpp"

namespace SpacePosts
{

    /**
     * @brief Test fixture which provides a StorageDirectorySetup for each test case.
     *
     * The StorageDirectorySetup is injected into the Tester object which is then used to call the parameterized
     * test method. For every test defined with TEST_P in main.cpp, GoogleTest instantiates a test case
     * for each tuple of parameters provdied by StorageStateProvider. Thus, every test is executed
     * with every paramerterization provided by StorageStateProvider.
     *
     * The parameters in this fixture are:
     * 1. the number of SpacePosts in the directory
     * 2. the index of the first SpacePost
     * 3. a generator function to define the indices of all following messages:
     *      Called once for each SpacePost to set the index of the next message
     * 4. Names of other files existing in the directory:
     *      Defines how many non-SpacePost files files exist in the directory and their names.
     *
     * Each combination of these parameters defines a different state of the storage directory.
     *
     */
    class StorageStateProvider : public testing::TestWithParam<std::tuple<U32, U32, std::function<U32()>,
                                                                          std::vector<std::string>>>
    {
    public:
        /**
         * @brief Construct an StorageStateProvider instance.
         *
         * The parameters are given by GoogleTest in the GetParams() method.
         * They define one unique state of the storage directory.
         */
        StorageStateProvider() : directorySetup(std::get<0>(GetParam()), std::get<1>(GetParam()),
                                                std::get<2>(GetParam()), std::get<3>(GetParam())),
                                 tester(directorySetup)
        {
        }

    protected:
        const SpacePosts::StorageDirectorySetup directorySetup;
        SpacePosts::Tester tester;
    };

    /**
     * @brief A child class of StorageStateProvider to distinguish between different groups of tests.
     *
     * This class is used to group tests which need to test all possible combinations (cartesian product)
     * of values of the **Boundary Value Analysis** for parameters that define the state of the storage directory.
     */
    class StorageStateProviderDetailed : public StorageStateProvider
    {
    };

    /**
     * @brief A child class of StorageStateProvider to distinguish between different groups of tests.
     *
     * This class is used to group tests which do not need to test all possible combinations of parameter values that
     * define the state of the storage directory.
     * Instead, they test with less combinations to reduce the number of test cases which would otherwise be large
     * due to the cartesian product.
     *
     * This group is only provided with a subset of representative examples from the cartesian product. This subset
     * includes the most important parameter value combinations and thus still provides a good branch coverage.
     */
    class StorageStateProviderCompact : public StorageStateProvider
    {
    };

    /**
     * @brief Test data for an elaborate coverage of the different states of the storage directory.
     *
     * The set of parameter values to instantiate the group of StorageStateProviderDetailed group.
     * 
     * Is formed from the cross product of all **Boundary Value Analysis** values per parameter.
     */
    const ::testing::internal::ParamGenerator<StorageStateProviderDetailed::ParamType>
        directorySetupParameterDetailed = ::testing::Combine(

            // Number of SpacePosts in the directory
            ::testing::Values(
                0,
                1,
                STest::Pick::lowerUpper(2, MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE - 2),
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE - 1,
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE,
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE + 1,
                STest::Pick::lowerUpper(MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE + 2, 1000)),

            // Index of the first SpacePost
            ::testing::Values(
                0,
                1,
                STest::Pick::lowerUpper(2, 1000000000)),
                // Max index not tested because component is not required to handle a index overflow correctly

            // Message index step generator starting from first message
            ::testing::Values(
                []()
                { return 1; },
                []()
                { return STest::Pick::lowerUpper(1, 5); },
                []()
                { return STest::Pick::lowerUpper(2, 1000); }),

            // Names of other files existing in the directory
            ::testing::Values(
                std::vector<std::string>{},                // No other files
                std::vector<std::string>{".", "hans"},     // Neither index nor extension
                std::vector<std::string>{"otherFile.ext"}, // Incorrect index and extension

                std::vector<std::string>{".spacepost"},                                          // No index
                std::vector<std::string>{"asdfasf.spacepost", "-324.spacepost", "222\n223.spacepost"}, // Incorrect index

                std::vector<std::string>{"1"},                            // No extension
                std::vector<std::string>{"21.spac", "0.msg", "0.dsf\00x"}, // Incorrect extension

                std::vector<std::string>{"otherFile.ext", "asdfasf.spacepst", "21.spac"} // Mixed multiple other files
                ));

    /**
     * @brief Test data for a more compact set of states of the storage directory that still covers most of the
     * component's behavior.
     *
     * The set of parameter values to instantiate the group of StorageStateProviderDetailed group.
     */
    const testing::internal::ParamGenerator<StorageStateProviderCompact::ParamType>
        directorySetupParameterCompact = ::testing::Values(
            // Empty directory
            std::make_tuple(
                0, 0,
                [](){ return 1; },
                std::vector<std::string>{}
            ),

            // Directory with one SpacePost
            std::make_tuple(
                1, 0,
                [](){ return 1; },
                std::vector<std::string>{}
            ),

            // Directory with only another file
            std::make_tuple(
                0, 0,
                [](){ return 1; },
                std::vector<std::string>{"otherFile.ext"}
            ),

            // Directory with one SpacePost and another file
            std::make_tuple(
                1, 0,
                [](){ return 1; },
                std::vector<std::string>{"mal\00for.me}d file name \34"}
            ),

            // Directory with typical number of SpacePosts
            std::make_tuple(
                25, 1,
                [](){ return STest::Pick::lowerUpper(1, 1000); },
                std::vector<std::string>{}
            ),

            // Directory with MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE - 1 many SpacePosts
            std::make_tuple(
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE - 1, 42,
                [](){ return STest::Pick::lowerUpper(1, 20); },
                std::vector<std::string>{".otherFile", "asdfasf.spacepost", "21.space", " "}
            ),

            // Directory with MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE many SpacePosts
            std::make_tuple(
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE, 0xefffffff,
                [](){ return STest::Pick::lowerUpper(1, 1000); },
                std::vector<std::string>{}
            ),

            // Directory with MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE + 1 many SpacePosts
            std::make_tuple(
                MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE + 1, 1,
                [](){ return STest::Pick::lowerUpper(1000, 10000); },
                std::vector<std::string>{".", "spacePostmsg"}
            ),

            // Directory with a lot more than MESSAGESTORAGE_STORED_INDEX_HISTORY_SIZE many SpacePosts
            std::make_tuple(
                333, 0,
                [](){ return STest::Pick::lowerUpper(1, 2); },
                std::vector<std::string>{"22"}
            )
        );

}
#endif // REF_MESSAGESTORAGE_TEST_UT_DATA_STORAGEDIRECTORYSTATES_HPP