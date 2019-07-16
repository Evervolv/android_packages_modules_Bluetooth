/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "packet/parser/test/test_packets.h"

#include <gtest/gtest.h>
#include <forward_list>
#include <memory>

#include "os/log.h"
#include "packet/bit_inserter.h"
#include "packet/parser/test/six_bytes.h"
#include "packet/raw_builder.h"

using ::bluetooth::packet::BitInserter;
using ::bluetooth::packet::kLittleEndian;
using ::bluetooth::packet::RawBuilder;
using ::bluetooth::packet::parser::test::SixBytes;
using std::vector;

namespace {
vector<uint8_t> child_two_two_three = {
    0x20 /* Reserved : 4, FourBits::TWO */,
    0x03 /* FourBits::THREE, Reserved : 4 */,
};
vector<uint8_t> child = {
    0x12 /* fixed */, 0x02 /* Size of the payload */, 0xa1 /* First byte of the payload */, 0xa2, 0xb1 /* footer */,
};
vector<uint8_t> child_with_six_bytes = {
    0x34 /* TwoBytes */,
    0x12,
    0xa1 /* First byte of the six_bytes */,
    0xa2,
    0xa3,
    0xa4,
    0xa5,
    0xa6,
    0xb1 /* Second six_bytes*/,
    0xb2,
    0xb3,
    0xb4,
    0xb5,
    0xb6,
};

}  // namespace

namespace bluetooth {
namespace packet {
namespace parser {
using namespace test;

TEST(GeneratedPacketTest, testChildTwoTwoThree) {
  auto packet = ChildTwoTwoThreeBuilder::Create();

  ASSERT_EQ(child_two_two_three.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child_two_two_three.size());
  for (size_t i = 0; i < child_two_two_three.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child_two_two_three[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentView wrong_view = ParentView::Create(packet_bytes_view);
  ASSERT_FALSE(wrong_view.IsValid());

  ParentTwoView parent_view = ParentTwoView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(FourBits::TWO, parent_view.GetFourBits());

  ChildTwoTwoView child_view = ChildTwoTwoView::Create(parent_view);
  ASSERT_TRUE(child_view.IsValid());
  ASSERT_EQ(FourBits::THREE, child_view.GetMoreBits());

  ChildTwoTwoThreeView grandchild_view = ChildTwoTwoThreeView::Create(child_view);
  ASSERT_TRUE(grandchild_view.IsValid());
}

TEST(GeneratedPacketTest, testChild) {
  uint16_t field_name = 0xa2a1;
  uint8_t footer = 0xb1;
  auto packet = ChildBuilder::Create(field_name, footer);

  ASSERT_EQ(child.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child.size());
  for (size_t i = 0; i < child.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentView parent_view = ParentView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  auto payload = parent_view.GetPayload();

  ASSERT_EQ(child[1 /* skip fixed field */], payload.size());
  for (size_t i = 0; i < payload.size(); i++) {
    ASSERT_EQ(child[i + 2 /* fixed & size */], payload[i]);
  }

  ChildView child_view = ChildView::Create(parent_view);
  ASSERT_TRUE(child_view.IsValid());

  ASSERT_EQ(field_name, child_view.GetFieldName());
}

TEST(GeneratedPacketTest, testValidateWayTooSmall) {
  std::vector<uint8_t> too_small_bytes = {0x34};
  auto too_small = std::make_shared<std::vector<uint8_t>>(too_small_bytes.begin(), too_small_bytes.end());

  ParentWithSixBytesView invalid_parent = ParentWithSixBytesView::Create(too_small);
  ASSERT_FALSE(invalid_parent.IsValid());
  ChildWithSixBytesView invalid = ChildWithSixBytesView::Create(ParentWithSixBytesView::Create(too_small));
  ASSERT_FALSE(invalid.IsValid());
}

TEST(GeneratedPacketTest, testValidateTooSmall) {
  std::vector<uint8_t> too_small_bytes = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x11};
  auto too_small = std::make_shared<std::vector<uint8_t>>(too_small_bytes.begin(), too_small_bytes.end());

  ParentWithSixBytesView valid_parent = ParentWithSixBytesView::Create(too_small);
  ASSERT_TRUE(valid_parent.IsValid());
  ChildWithSixBytesView invalid = ChildWithSixBytesView::Create(ParentWithSixBytesView::Create(too_small));
  ASSERT_FALSE(invalid.IsValid());
}

TEST(GeneratedPacketTest, testValidateJustRight) {
  std::vector<uint8_t> just_right_bytes = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0x05,
                                           0x06, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
  auto just_right = std::make_shared<std::vector<uint8_t>>(just_right_bytes.begin(), just_right_bytes.end());

  ChildWithSixBytesView valid = ChildWithSixBytesView::Create(ParentWithSixBytesView::Create(just_right));
  ASSERT_TRUE(valid.IsValid());
}

TEST(GeneratedPacketTest, testValidateTooBig) {
  std::vector<uint8_t> too_big_bytes = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20};
  auto too_big = std::make_shared<std::vector<uint8_t>>(too_big_bytes.begin(), too_big_bytes.end());

  ChildWithSixBytesView lenient = ChildWithSixBytesView::Create(ParentWithSixBytesView::Create(too_big));
  ASSERT_TRUE(lenient.IsValid());
}

TEST(GeneratedPacketTest, testValidateDeath) {
  auto packet = ChildTwoTwoThreeBuilder::Create();

  ASSERT_EQ(child_two_two_three.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child_two_two_three.size());
  for (size_t i = 0; i < child_two_two_three.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child_two_two_three[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentView wrong_view = ParentView::Create(packet_bytes_view);
  ASSERT_DEATH(wrong_view.GetPayload(), "validated");
}

TEST(GeneratedPacketTest, testValidatedParentDeath) {
  uint16_t field_name = 0xa2a1;
  uint8_t footer = 0xb1;
  auto packet = ChildBuilder::Create(field_name, footer);

  ASSERT_EQ(child.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child.size());
  for (size_t i = 0; i < child.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentView parent_view = ParentView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  auto payload = parent_view.GetPayload();

  ASSERT_EQ(child[1 /* skip fixed field */], payload.size());
  for (size_t i = 0; i < payload.size(); i++) {
    ASSERT_EQ(child[i + 2 /* fixed & size */], payload[i]);
  }

  ChildView child_view = ChildView::Create(parent_view);
  ASSERT_DEATH(child_view.GetFieldName(), "validated");
}

vector<uint8_t> middle_four_bits = {
    0x95,  // low_two = ONE, next_four = FIVE, straddle = TEN
    0x8a,  // straddle = TEN, four_more = TWO, high_two = TWO
};

TEST(GeneratedPacketTest, testMiddleFourBitsPacket) {
  TwoBits low_two = TwoBits::ONE;
  FourBits next_four = FourBits::FIVE;
  FourBits straddle = FourBits::TEN;
  FourBits four_more = FourBits::TWO;
  TwoBits high_two = TwoBits::TWO;

  auto packet = MiddleFourBitsBuilder::Create(low_two, next_four, straddle, four_more, high_two);

  ASSERT_EQ(middle_four_bits.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), middle_four_bits.size());
  for (size_t i = 0; i < middle_four_bits.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), middle_four_bits[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  MiddleFourBitsView view = MiddleFourBitsView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  ASSERT_EQ(low_two, view.GetLowTwo());
  ASSERT_EQ(next_four, view.GetNextFour());
  ASSERT_EQ(straddle, view.GetStraddle());
  ASSERT_EQ(four_more, view.GetFourMore());
  ASSERT_EQ(high_two, view.GetHighTwo());
}

TEST(GeneratedPacketTest, testChildWithSixBytes) {
  SixBytes six_bytes_a{{0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6}};
  SixBytes six_bytes_b{{0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6}};
  auto packet = ChildWithSixBytesBuilder::Create(six_bytes_a, six_bytes_b);

  ASSERT_EQ(child_with_six_bytes.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child_with_six_bytes.size());
  for (size_t i = 0; i < child_with_six_bytes.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child_with_six_bytes[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentWithSixBytesView parent_view = ParentWithSixBytesView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(six_bytes_a, parent_view.GetSixBytes());

  ChildWithSixBytesView child_view = ChildWithSixBytesView::Create(parent_view);
  ASSERT_TRUE(child_view.IsValid());

  ASSERT_EQ(six_bytes_a, child_view.GetSixBytes());
  ASSERT_EQ(six_bytes_a, ((ParentWithSixBytesView)child_view).GetSixBytes());
  ASSERT_EQ(six_bytes_b, child_view.GetChildSixBytes());
}

namespace {
vector<uint8_t> parent_with_sum = {
    0x11 /* TwoBytes */, 0x12, 0x21 /* Sum Bytes */, 0x22, 0x43 /* Sum, excluding TwoBytes */, 0x00,
};

}  // namespace

TEST(GeneratedPacketTest, testParentWithSum) {
  uint16_t two_bytes = 0x1211;
  uint16_t sum_bytes = 0x2221;
  auto packet = ParentWithSumBuilder::Create(two_bytes, sum_bytes, std::make_unique<packet::RawBuilder>());

  ASSERT_EQ(parent_with_sum.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), parent_with_sum.size());
  for (size_t i = 0; i < parent_with_sum.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), parent_with_sum[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentWithSumView parent_view = ParentWithSumView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(two_bytes, parent_view.GetTwoBytes());

  // Corrupt checksum
  packet_bytes->back()++;
  PacketView<kLittleEndian> corrupted_bytes_view(packet_bytes);
  ParentWithSumView corrupted_view = ParentWithSumView::Create(corrupted_bytes_view);
  ASSERT_FALSE(corrupted_view.IsValid());
}

namespace {
vector<uint8_t> child_with_nested_sum = {
    0x11 /* TwoBytes */,
    0x12,
    0x21 /* Sum Bytes */,
    0x22,
    0x31 /* More Bytes */,
    0x32,
    0x33,
    0x34,
    0xca /* Nested Sum */,
    0x00,
    0xd7 /* Sum, excluding TwoBytes */,
    0x01,
};

}  // namespace

TEST(GeneratedPacketTest, testChildWithNestedSum) {
  uint16_t two_bytes = 0x1211;
  uint16_t sum_bytes = 0x2221;
  uint32_t more_bytes = 0x34333231;
  auto packet = ChildWithNestedSumBuilder::Create(two_bytes, sum_bytes, more_bytes);

  ASSERT_EQ(child_with_nested_sum.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(packet_bytes->size(), child_with_nested_sum.size());
  for (size_t i = 0; i < child_with_nested_sum.size(); i++) {
    ASSERT_EQ(packet_bytes->at(i), child_with_nested_sum[i]);
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentWithSumView parent_view = ParentWithSumView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(two_bytes, parent_view.GetTwoBytes());

  ChildWithNestedSumView child_view = ChildWithNestedSumView::Create(parent_view);
  ASSERT_TRUE(child_view.IsValid());

  ASSERT_EQ(more_bytes, child_view.GetMoreBytes());
}

namespace {
vector<uint8_t> parent_size_modifier = {
    0x02 /* Size */,
    0x11 /* TwoBytes */,
    0x12,
};

}  // namespace

TEST(GeneratedPacketTest, testParentSizeModifier) {
  uint16_t two_bytes = 0x1211;
  auto packet = ParentSizeModifierBuilder::Create(std::make_unique<RawBuilder>(), two_bytes);

  ASSERT_EQ(parent_size_modifier.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(parent_size_modifier.size(), packet_bytes->size());
  for (size_t i = 0; i < parent_size_modifier.size(); i++) {
    ASSERT_EQ(parent_size_modifier[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentSizeModifierView parent_view = ParentSizeModifierView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(two_bytes, parent_view.GetTwoBytes());
}

namespace {
vector<uint8_t> child_size_modifier = {
    0x06 /* PayloadSize (TwoBytes + MoreBytes)*/,
    0x31 /* MoreBytes */,
    0x32,
    0x33,
    0x34,
    0x11 /* TwoBytes = 0x1211 */,
    0x12,
};

}  // namespace

TEST(GeneratedPacketTest, testChildSizeModifier) {
  uint16_t two_bytes = 0x1211;
  uint32_t more_bytes = 0x34333231;
  auto packet = ChildSizeModifierBuilder::Create(more_bytes);

  ASSERT_EQ(child_size_modifier.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(child_size_modifier.size(), packet_bytes->size());
  for (size_t i = 0; i < child_size_modifier.size(); i++) {
    ASSERT_EQ(child_size_modifier[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  ParentSizeModifierView parent_view = ParentSizeModifierView::Create(packet_bytes_view);
  ASSERT_TRUE(parent_view.IsValid());
  ASSERT_EQ(two_bytes, parent_view.GetTwoBytes());

  ChildSizeModifierView child_view = ChildSizeModifierView::Create(parent_view);
  ASSERT_TRUE(child_view.IsValid());

  ASSERT_EQ(more_bytes, child_view.GetMoreBytes());
}

namespace {
vector<uint8_t> fixed_array_enum{
    0x01,  // ONE
    0x00,
    0x02,  // TWO
    0x00,
    0x01,  // ONE_TWO
    0x02,
    0x02,  // TWO_THREE
    0x03,
    0xff,  // FFFF
    0xff,
};
}

TEST(GeneratedPacketTest, testFixedArrayEnum) {
  std::vector<ForArrays> fixed_array{
      {ForArrays::ONE, ForArrays::TWO, ForArrays::ONE_TWO, ForArrays::TWO_THREE, ForArrays::FFFF}};
  auto packet = FixedArrayEnumBuilder::Create(fixed_array);
  ASSERT_EQ(fixed_array_enum.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(fixed_array_enum.size(), packet_bytes->size());
  for (size_t i = 0; i < fixed_array_enum.size(); i++) {
    ASSERT_EQ(fixed_array_enum[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = FixedArrayEnumView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetEnumArray();
  ASSERT_EQ(fixed_array.size(), array.size());
  for (size_t i = 0; i < fixed_array.size(); i++) {
    ASSERT_EQ(array[i], fixed_array[i]);
  }
}

namespace {
vector<uint8_t> sized_array_enum{
    0x0a,  // _size_
    0x00,
    0x01,  // ONE
    0x00,
    0x02,  // TWO
    0x00,
    0x01,  // ONE_TWO
    0x02,
    0x02,  // TWO_THREE
    0x03,
    0xff,  // FFFF
    0xff,
};
}

TEST(GeneratedPacketTest, testSizedArrayEnum) {
  std::vector<ForArrays> sized_array{
      {ForArrays::ONE, ForArrays::TWO, ForArrays::ONE_TWO, ForArrays::TWO_THREE, ForArrays::FFFF}};
  auto packet = SizedArrayEnumBuilder::Create(sized_array);
  ASSERT_EQ(sized_array_enum.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(sized_array_enum.size(), packet_bytes->size());
  for (size_t i = 0; i < sized_array_enum.size(); i++) {
    ASSERT_EQ(sized_array_enum[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = SizedArrayEnumView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetEnumArray();
  ASSERT_EQ(sized_array.size(), array.size());
  for (size_t i = 0; i < sized_array.size(); i++) {
    ASSERT_EQ(array[i], sized_array[i]);
  }
}

namespace {
vector<uint8_t> count_array_enum{
    0x03,  // _count_
    0x01,  // ONE
    0x00,
    0x02,  // TWO_THREE
    0x03,
    0xff,  // FFFF
    0xff,
};
}

TEST(GeneratedPacketTest, testCountArrayEnum) {
  std::vector<ForArrays> count_array{{ForArrays::ONE, ForArrays::TWO_THREE, ForArrays::FFFF}};
  auto packet = CountArrayEnumBuilder::Create(count_array);
  ASSERT_EQ(count_array_enum.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(count_array_enum.size(), packet_bytes->size());
  for (size_t i = 0; i < count_array_enum.size(); i++) {
    ASSERT_EQ(count_array_enum[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = CountArrayEnumView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetEnumArray();
  ASSERT_EQ(count_array.size(), array.size());
  for (size_t i = 0; i < count_array.size(); i++) {
    ASSERT_EQ(array[i], count_array[i]);
  }
}

TEST(GeneratedPacketTest, testFixedSizeByteArray) {
  constexpr int byte_array_size = 32;
  std::vector<uint8_t> byte_array(byte_array_size);
  for (uint8_t i = 0; i < byte_array_size; i++) byte_array[i] = i;

  constexpr int word_array_size = 8;
  std::vector<uint32_t> word_array(word_array_size);
  for (uint32_t i = 0; i < word_array_size; i++) word_array[i] = i;

  auto packet = PacketWithFixedArraysOfBytesBuilder::Create(byte_array, word_array);
  ASSERT_EQ(2 * (256 / 8), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(byte_array_size + word_array_size * sizeof(uint32_t), packet_bytes->size());

  for (size_t i = 0; i < byte_array_size; i++) {
    ASSERT_EQ(byte_array[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = PacketWithFixedArraysOfBytesView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetFixed256bitInBytes();
  ASSERT_EQ(byte_array.size(), array.size());
  for (size_t i = 0; i < array.size(); i++) {
    ASSERT_EQ(array[i], byte_array[i]);
  }

  auto decoded_word_array = view.GetFixed256bitInWords();
  ASSERT_EQ(word_array.size(), decoded_word_array.size());
  for (size_t i = 0; i < decoded_word_array.size(); i++) {
    ASSERT_EQ(word_array[i], decoded_word_array[i]);
  }
}

TEST(GeneratedPacketTest, testFixedSizeArrayValidatorDeath) {
  constexpr int byte_array_size = 33;
  std::vector<uint8_t> byte_array(byte_array_size);
  for (uint8_t i = 0; i < byte_array_size; i++) byte_array[i] = i;

  constexpr int word_array_size = 8;
  std::vector<uint32_t> word_array(word_array_size);
  for (uint32_t i = 0; i < word_array_size; i++) word_array[i] = i;

  std::unique_ptr<PacketWithFixedArraysOfBytesBuilder> packet;
  ASSERT_DEATH(packet = PacketWithFixedArraysOfBytesBuilder::Create(byte_array, word_array), "size");
}

vector<uint8_t> one_variable{
    0x03, 'o', 'n', 'e',  // "one"
};

TEST(GeneratedPacketTest, testOneVariableField) {
  std::vector<Variable> sized_array;
  sized_array.emplace_back("one");

  auto packet = OneVariableBuilder::Create(sized_array[0]);
  ASSERT_EQ(one_variable.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(one_variable.size(), packet_bytes->size());
  for (size_t i = 0; i < one_variable.size(); i++) {
    ASSERT_EQ(one_variable[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = OneVariableView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetOne();
  ASSERT_EQ(sized_array.size(), array.size());
  for (size_t i = 0; i < sized_array.size(); i++) {
    ASSERT_EQ(array[i].data, sized_array[i].data);
  }
}

vector<uint8_t> sized_array_variable{
    0x0e,                           // _size_
    0x03, 'o', 'n', 'e',            // "one"
    0x03, 't', 'w', 'o',            // "two"
    0x05, 't', 'h', 'r', 'e', 'e',  // "three"
};

TEST(GeneratedPacketTest, testSizedArrayVariableLength) {
  std::vector<Variable> sized_array;
  sized_array.emplace_back("one");
  sized_array.emplace_back("two");
  sized_array.emplace_back("three");

  auto packet = SizedArrayVariableBuilder::Create(sized_array);
  ASSERT_EQ(sized_array_variable.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(sized_array_variable.size(), packet_bytes->size());
  for (size_t i = 0; i < sized_array_variable.size(); i++) {
    ASSERT_EQ(sized_array_variable[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = SizedArrayVariableView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetVariableArray();
  ASSERT_EQ(sized_array.size(), array.size());
  for (size_t i = 0; i < sized_array.size(); i++) {
    ASSERT_EQ(array[i].data, sized_array[i].data);
  }
}

vector<uint8_t> fixed_array_variable{
    0x03, 'o', 'n', 'e',            // "one"
    0x03, 't', 'w', 'o',            // "two"
    0x05, 't', 'h', 'r', 'e', 'e',  // "three"
    0x04, 'f', 'o', 'u', 'r',       // "four"
    0x04, 'f', 'i', 'v', 'e',       // "five"
};

TEST(GeneratedPacketTest, testFixedArrayVariableLength) {
  std::vector<Variable> fixed_array;
  fixed_array.emplace_back("one");
  fixed_array.emplace_back("two");
  fixed_array.emplace_back("three");
  fixed_array.emplace_back("four");
  fixed_array.emplace_back("five");

  auto packet = FixedArrayVariableBuilder::Create(fixed_array);
  ASSERT_EQ(fixed_array_variable.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(fixed_array_variable.size(), packet_bytes->size());
  for (size_t i = 0; i < fixed_array_variable.size(); i++) {
    ASSERT_EQ(fixed_array_variable[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = FixedArrayVariableView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetVariableArray();
  ASSERT_EQ(fixed_array.size(), array.size());
  for (size_t i = 0; i < fixed_array.size(); i++) {
    ASSERT_EQ(array[i].data, fixed_array[i].data);
  }
}

vector<uint8_t> count_array_variable{
    0x04,                           // _count_
    0x03, 'o', 'n', 'e',            // "one"
    0x03, 't', 'w', 'o',            // "two"
    0x05, 't', 'h', 'r', 'e', 'e',  // "three"
    0x04, 'f', 'o', 'u', 'r',       // "four"
};

TEST(GeneratedPacketTest, testCountArrayVariableLength) {
  std::vector<Variable> count_array;
  count_array.emplace_back("one");
  count_array.emplace_back("two");
  count_array.emplace_back("three");
  count_array.emplace_back("four");

  auto packet = CountArrayVariableBuilder::Create(count_array);
  ASSERT_EQ(count_array_variable.size(), packet->size());

  std::shared_ptr<std::vector<uint8_t>> packet_bytes = std::make_shared<std::vector<uint8_t>>();
  BitInserter it(*packet_bytes);
  packet->Serialize(it);

  ASSERT_EQ(count_array_variable.size(), packet_bytes->size());
  for (size_t i = 0; i < count_array_variable.size(); i++) {
    ASSERT_EQ(count_array_variable[i], packet_bytes->at(i));
  }

  PacketView<kLittleEndian> packet_bytes_view(packet_bytes);
  auto view = CountArrayVariableView::Create(packet_bytes_view);
  ASSERT_TRUE(view.IsValid());
  auto array = view.GetVariableArray();
  ASSERT_EQ(count_array.size(), array.size());
  for (size_t i = 0; i < count_array.size(); i++) {
    ASSERT_EQ(array[i].data, count_array[i].data);
  }
}
}  // namespace parser
}  // namespace packet
}  // namespace bluetooth
