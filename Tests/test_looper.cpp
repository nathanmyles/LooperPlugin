#include <gtest/gtest.h>
#include "../Source/Models/Looper.h"

TEST(LooperTest, InitialStateIsEmpty) {
    Looper looper;
    EXPECT_FALSE(looper.isRecording());
    EXPECT_FALSE(looper.isPlaying());
}

TEST(LooperTest, CanStartRecording) {
    Looper looper;
    looper.startRecording(0);
    EXPECT_TRUE(looper.isRecording());
}

TEST(LooperTest, CanStopRecording) {
    Looper looper;
    looper.startRecording(0);
    looper.stopRecording();
    EXPECT_FALSE(looper.isRecording());
}