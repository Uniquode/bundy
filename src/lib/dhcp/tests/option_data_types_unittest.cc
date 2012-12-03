// Copyright (C) 2012 Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <config.h>
#include <dhcp/option_data_types.h>
#include <gtest/gtest.h>

using namespace isc;
using namespace isc::dhcp;

namespace {

/// @brief Test class for option data type utilities.
class OptionDataTypesTest : public ::testing::Test {
public:

    /// @brief Constructor.
    OptionDataTypesTest() { }

    /// @brief Write IP address into a buffer.
    ///
    /// @param address address to be written.
    /// @param [out] buf output buffer.
    void writeAddress(const asiolink::IOAddress& address,
                      std::vector<uint8_t>& buf) {
        short family = address.getFamily();
        if (family == AF_INET) {
            asio::ip::address_v4::bytes_type buf_addr =
                address.getAddress().to_v4().to_bytes();
            buf.insert(buf.end(), buf_addr.begin(), buf_addr.end());
        } else if (family == AF_INET6) {
            asio::ip::address_v6::bytes_type buf_addr =
                address.getAddress().to_v6().to_bytes();
            buf.insert(buf.end(), buf_addr.begin(), buf_addr.end());
        }
    }

    /// @brief Write integer (signed or unsigned) into a buffer.
    ///
    /// @param value integer value.
    /// @param [out] buf output buffer.
    /// @tparam integer type.
    template<typename T>
    void writeInt(T value, std::vector<uint8_t>& buf) {
        for (int i = 0; i < sizeof(T); ++i) {
            buf.push_back(value >> ((sizeof(T) - i - 1) * 8) & 0xFF);
        }
    }

    /// @brief Write a string into a buffer.
    ///
    /// @param value string to be written into a buffer.
    /// @param buf output buffer.
    void writeString(const std::string& value,
                     std::vector<uint8_t>& buf) {
        buf.resize(buf.size() + value.size());
        std::copy_backward(value.c_str(), value.c_str() + value.size(),
                           buf.end());
    }
};

// The goal of this test is to verify that an IPv4 address being
// stored in a buffer (wire format) can be read into IOAddress
// object.
TEST_F(OptionDataTypesTest, readAddress) {
    // Create some IPv4 address.
    asiolink::IOAddress address("192.168.0.1");
    // And store it in a buffer in a wire format.
    std::vector<uint8_t> buf;
    writeAddress(address, buf);

    // Now, try to read the IP address with a utility function
    // being under test.
    asiolink::IOAddress address_out("127.0.0.1");
    EXPECT_NO_THROW(address_out = OptionDataTypeUtil::readAddress(buf, AF_INET));

    // Check that the read address matches address that
    // we used as input.
    EXPECT_EQ(address.toText(), address_out.toText());
}

// The goal of this test is to verify that an IPv6 address
// is properly converted to wire format and stored in a
// buffer.
TEST_F(OptionDataTypesTest, writeAddress) {
    // Encode an IPv6 address 2001:db8:1::1 in wire format.
    // This will be used as reference data to validate if
    // an IPv6 address is stored in a buffer properly.
    const char data[] = {
        0x20, 0x01, 0x0d, 0xb8, 0x0, 0x1, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
    };
    std::vector<uint8_t> buf_in(data, data + sizeof(data));

    // Create IPv6 address object.
    asiolink::IOAddress address("2001:db8:1::1");
    // Define the output buffer to write IP address to.
    std::vector<uint8_t> buf_out;
    // Write the address to the buffer.
    ASSERT_NO_THROW(OptionDataTypeUtil::writeAddress(address, buf_out));
    // Make sure that input and output buffers have the same size
    // so we can compare them.
    ASSERT_EQ(buf_in.size(), buf_out.size());
    // And finally compare them.
    EXPECT_TRUE(std::equal(buf_in.begin(), buf_in.end(), buf_out.begin()));
}

// The purpose of this test is to verify that FQDN is read from
// a buffer and returned as a text. The representation of the FQDN
// in the buffer complies with RFC1035, section 3.1.
// This test also checks that if invalid (truncated) FQDN is stored
// in a buffer the appropriate exception is returned when trying to
// read it as a string.
TEST_F(OptionDataTypesTest, readFqdn) {
    // The binary representation of the "mydomain.example.com".
    // Values: 8, 7, 3 and 0 specify the lengths of subsequent
    // labels within the FQDN.
    const char data[] = {
        8, 109, 121, 100, 111, 109, 97, 105, 110, // "mydomain"
        7, 101, 120, 97, 109, 112, 108, 101,      // "example"
        3, 99, 111, 109,                          // "com"
        0
    };

    // Make a vector out of the data.
    std::vector<uint8_t> buf(data, data + sizeof(data));

    // Read the buffer as FQDN and verify its correctness.
    std::string fqdn;
    size_t len = 0;
    EXPECT_NO_THROW(fqdn = OptionDataTypeUtil::readFqdn(buf, len));
    EXPECT_EQ("mydomain.example.com.", fqdn);
    EXPECT_EQ(len, buf.size());

    // By resizing the buffer we simulate truncation. The first
    // length field (8) indicate that the first label's size is
    // 8 but the actual buffer size is 5. Expect that conversion
    // fails.
    buf.resize(5);
    EXPECT_THROW(
        OptionDataTypeUtil::readFqdn(buf, len),
        isc::dhcp::BadDataTypeCast
    );

    // Another special case: provide an empty buffer.
    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::readFqdn(buf, len),
        isc::dhcp::BadDataTypeCast
    );
}

// The purpose of this test is to verify that FQDN's syntax is validated
// and that FQDN is correctly written to a buffer in a format described
// in RFC1035 section 3.1.
TEST_F(OptionDataTypesTest, writeFqdn) {
    // Create empty buffer. The FQDN will be written to it.
    OptionBuffer buf;
    // Write a domain name into the buffer in the format described
    // in RFC1035 section 3.1. This function should not throw
    // exception because domain name is well formed.
    EXPECT_NO_THROW(
        OptionDataTypeUtil::writeFqdn("mydomain.example.com", buf)
    );
    // The length of the data is 22 (8 bytes for "mydomain" label,
    // 7 bytes for "example" label, 3 bytes for "com" label and
    // finally 4 bytes positions between labels where length
    // information is stored.
    ASSERT_EQ(22, buf.size());

    // Verify that length fields between labels hold valid values.
    EXPECT_EQ(8, buf[0]);  // length of "mydomain"
    EXPECT_EQ(7, buf[9]);  // length of "example"
    EXPECT_EQ(3, buf[17]); // length of "com"
    EXPECT_EQ(0, buf[21]); // zero byte at the end.

    // Verify that labels are valid.
    std::string label0(buf.begin() + 1, buf.begin() + 9);
    EXPECT_EQ("mydomain", label0);

    std::string label1(buf.begin() + 10, buf.begin() + 17);
    EXPECT_EQ("example", label1);

    std::string label2(buf.begin() + 18, buf.begin() + 21);
    EXPECT_EQ("com", label2);

    // The tested function is supposed to append data to a buffer
    // so let's check that it is a case by appending another domain.
    OptionDataTypeUtil::writeFqdn("hello.net", buf);

    // The buffer length should be now longer.
    ASSERT_EQ(33, buf.size());

    // Check the length fields for new labels being appended.
    EXPECT_EQ(5, buf[22]);
    EXPECT_EQ(3, buf[28]);

    // And check that labels are ok.
    std::string label3(buf.begin() + 23, buf.begin() + 28);
    EXPECT_EQ("hello", label3);

    std::string label4(buf.begin() + 29, buf.begin() + 32);
    EXPECT_EQ("net", label4);

    // Check that invalid (empty) FQDN is rejected and expected
    // exception type is thrown.
    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::writeFqdn("", buf),
        isc::dhcp::BadDataTypeCast
    );

    // Check another invalid domain name (with repeated dot).
    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::writeFqdn("example..com", buf),
        isc::dhcp::BadDataTypeCast
    );
}

} // anonymous namespace
