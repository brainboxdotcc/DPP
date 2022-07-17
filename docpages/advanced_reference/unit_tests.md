\page unit-tests Unit Tests

## Running Unit Tests

If you are adding functionality to DPP, make sure to run unit tests. This makes sure that the changes do not break anything. All pull requests must pass all unit tests before merging.

Before running test cases, create a test server for your test bot. You should:

* Make sure that the server only has you and your test bot, and no one else
* Give your bot the administrator permission
* Enable community for the server
* Make an event
* Create at least one voice channel
* Create at least one text channel

Then, set the following variables to the appropriate values. (Below is a fake token, don't bother trying to use it)

    export DPP_UNIT_TEST_TOKEN="ODI2ZSQ4CFYyMzgxUzkzzACy.HPL5PA.9qKR4uh8po63-pjYVrPAvQQO4ln"
    export TEST_GUILD_ID="907951970017480704"
    export TEST_TEXT_CHANNEL_ID="907951970017480707"
    export TEST_VC_ID="907951970017480708"
    export TEST_USER_ID="826535422381391913"
    export TEST_EVENT_ID="909928577951203360"

Then, after cloning and building DPP, run `cd build && ctest -VV` for unit test cases. 

If you do not specify the `DPP_UNIT_TEST_TOKEN` environment variable, a subset of the tests will run which do not require discord connectivity.

