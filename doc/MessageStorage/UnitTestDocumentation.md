# BBSMessageStorage Unit Test Documentation

## Summary

- The BBSMessageStorage component has been unit tested to X% line coverage and X% branch coverage.
- The unit tests follow the data-driven unit test style.
- The unit tests are implemented with the GoogleTest testing library.
- Both black-box and white-box unit tests are used to make the unit tests as independent from the component as possible while still covering all internal error-handling branches.
- An object-oriented model is used to simplify the unit tests by modeling helper data and functionality in classes.

## Table of Contents
- [BBSMessageStorage Unit Test Documentation](#bbsmessagestorage-unit-test-documentation)
  - [Summary](#summary)
  - [Table of Contents](#table-of-contents)
  - [Test Success Criteria](#test-success-criteria)
  - [How To Navigate The Unit Test Code](#how-to-navigate-the-unit-test-code)
  - [Test Strategy](#test-strategy)
  - [Test Environment](#test-environment)
  - [List of Test Cases](#list-of-test-cases)
  - [Test List](#test-list)


## Test Success Criteria

- More than 90% line coverage
- More than 85% branch coverage
- Desired: High path coverage (not measured by our tools, though)

## How To Navigate The Unit Test Code

- The test logic is defined in the (Tester.hpp)[//TODO] and implemented in the (Tester.cpp)[//TODO].
- The test data is defined in the (main.cpp)[//TODO] and (TODO.hpp)[//TODO].
    - For unit tests with a small set of test data values, one test case is defined for every test data value combination as a one-liner with GoogleTest's `TEST_P()` syntax in (main.cpp)[//TODO]. For example, see //TODO. This makes the tests more readable.
    - For unit tests with a large set of test data values, a GoogleTest parameterized test fixture class is used to define a single test with `TEST_P()`. This test is then executed once for every test data value provided by the test fixture class. The fixture class's test data is defined in (TODO.hpp)[//TODO].
- An object-oriented model of message files and the storage directory facilitate simple unit test code. It is defined in //TODO

## Test Strategy

**Data-Driven Unit Testing**

The tests separate the logic that is tested from the data that is used for the test. The logic is defined in a parameterized test method (in the (Tester.hpp)[//TODO]) while the test data is defined in a test data provider class (in the (TODO.hpp)[//TODO]). A single test method is then executed once for every combination of test parameters provided by the test data provider class.

To achieve high path and branch coverage, the data-driven approach makes sense for the `MessageStorage` component. As the component is all about storing and loading data, its test cases rely on checking the component's behavior for many different messages.

**Black-Box Testing**

First, I searched for all black-box test cases that can be derived from the component's public interface. I **partitioned** the parameter domains **into** **equivalence classes** based on the domain limits and the intuitively assumed limits of processing.

Next, I developed **Boundary Value Analysis** tests for these partitions. This led to the black-box unit tests realized for the `MessageStorage` component (see [Black-Box Test Cases](//TODO)).

For every partition boundary, the value just before the boundary, the boundary value, and the value just past the boundary are tested. Additionally, for large equivalence classes, one random value from inside the partition is tested.

For example, in UT-STO-020, the boundaries of the message length are 0 and `MAX_MSGTEXT_LENGTH`. Thus, messages with length 0, 1, a random length in [2,MAX_MSGTEXT_LENGTH-2], MAX_MSGTEXT_LENGTH - 1, and MAX_MSGTEXT_LENGTH are tested.

From (`main.cpp`)[//TODO]

``` cpp
TEST_P(BBSMessageStorageTest, TestStoreMessageTextNominalEmpty)
{
    tester.testStoreMessageText("");
}

TEST_P(BBSMessageStorageTest, TestStoreMessageTextNominalSingleChar)
{
    tester.testStoreMessageText("x");
}

TEST_P(BBSMessageStorageTest, TestStoreMessageTextNominalTypicalMessageSizeAllChars)
{
    tester.testStoreMessageText(STest::Pick::stringNonNull(STest::Pick::lowerUpper(2, MAX_MSGTEXT_LENGTH - 1)));
}

TEST_P(BBSMessageStorageTest, TestStoreMessageTextNominalMaxMessageSize)
{
    tester.testStoreMessageText(STest::Pick::stringNonNull(MAX_MSGTEXT_LENGTH));
}

```

**White-Box Testing**

The black-box Boundary Value Analysis tests provide good line and branch coverage of the nominal component behavior. However, they cannot cover all error cases as, for file system interactions, there are a lot more possible errors beyond invalid input data.

Consequently, I developed a set of **control-flow-oriented tests** that uses white-box knowledge of which branches are taken under which error conditions. The tests intercept the component's file system interactions and consciously inject errors to test the error-handling branches in the source code of the component. Therefore, these tests can increase the line coverage over the black-box tests (see [White-Box Test Cases](//TODO)).

**Independence Assumption**

I additionally used white-box knowledge of the component's implementation to make sensible assumptions about which test method parameters can be changed without affecting other parameters. For example, it is reasonably safe to assume that the index at which a message is stored does not affect whether the component loads a certain message content correctly.

Consequently, the independence assumptions allow for simplifying the unit tests by testing changes in certain parameters separately from each other instead of testing all possible combinations of changes in these parameters. For example, this allowed us to first test whether the component correctly loads messages from different indices while keeping the message content constant (UT-STO-030 //TODO link). Analogously, a second test case then tests whether the component correctly loads messages with different content while keeping the index constant (UT-STO-040 //TODO link).

## Test Environment

**Test together with the OSAL**

The BBSMessageStorage component is tested together with the Operating System Abstraction Layer (OSAL) of F'. We consider the component plus the OSAL as one unit for this component's unit tests. This step is sensible because the OSAL is used internally in the component without any interface for users to change it. Furthermore, the interaction with the real OSAL is an integral part of the component and we can assume the correctness of the OSAL since it is part of the well-tested framework.

**Modern C++ Style**

The unit tests do not follow the embedded coding style as the source code does. This decision is advisable because the unit tests will not be executed in an embedded environment. Thus, modern C++ features can be used to reduce code complexity.

## List of Test Cases
## Test List

This list outlines all the unit test that have been implemented. For more detailed explanations of how the unit tests are realized, refer to the test method comments in Tester.hpp //TODO link.

**Black-Box Test**
| Test Case ID | Name | Steps | Variable Test Data | Realization |
| --- | --- | --- | --- | --- |
| UT-STO-010 | Test the assigned index to stored messages based on different state of the storage directory | 1. Setup storage directory with certain existing files, 2. Call component to store message, 3. Check that the assigned index to the message from what is reported via events and telemetry | directory does or does not exist, number of stored BBS messages, indices of the stored BBS messages, number of other files, naming of other files | Tester::testStore-Index(), Tester::testStoreIndex-DirDoesNotExist() |
| UT-STO-020 | Test storing a message based on the message’s text content | 1. Call component input port to store message, 2. Check that a corresponding file has been correctly stored on disk by reading and checking the created file | Message text’s length, message text’s content, storage directory states from UT-STO-010 | Tester::testStore-MessageText(),|
| UT-STO-030 | Test loading a message from a given index based on whether that index exists | 1. Call component input port to load a message from the given index, 2. Check whether loading succeeds or fails from the emitted events and telemetry | Index of message to load, storage directory states from UT-STO-010 | Tester::testLoadFrom-ExistingIndex(), testLoadFromNon-ExistingIndex() |
| UT-STO-040 | Test loading a message from a given index based on the validity of the file on disk referenced by the index | 1. Place a consciously formatted file for a BBS message on disk, 2. Call component input port to load a message from the index, 3. If invalid file: Check whether loading fails for the specific reason for which it should by checking the emitted events and telemetry. If valid file: Check whether returned message is the one that was stored in the message file | Message’s meta data, Message text’s length, Message text’s content, storage directory states from UT-STO-010 | Tester::testLoadValid-BBSFileFromIndex(), Tester::testLoadInvalid-BBSFileFromIndex() |
| UT-STO-050 | Test whether loading last N messages selects the most recently stored messages based on different numbers for N | 1. Setup storage directory with certain existing files, 2. Call component input port to load the last N messages, 3. Check whether the loaded messages are the ones that have the most recent indices in specified order  | Number of messages N to load, storage directory states from UT-STO-010 |  |
| UT-STO-060 | Test loading the last N messages based on the validity of the corresponding message files on disk | 1. Place consciously formatted files for BBS messages on disk as the last N message files, 2. Call component input port to load the last N messages, 3. Check whether invalid messages have been skipped in loading | Per placed message file: Message’s meta data, Message text’s length, Message text’s content; Number of messages N to load; Storage directory states from UT-STO-010; |  |