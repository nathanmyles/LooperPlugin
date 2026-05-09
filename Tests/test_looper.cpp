/*
 * LooperPlugin
 * Copyright (C) 2026 NathanMyles
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../Source/Models/Looper.h"
#include <gtest/gtest.h>

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
