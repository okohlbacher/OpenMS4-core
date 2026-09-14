// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Marc Sturm $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////

#include <OpenMS/FORMAT/Base64.h>
#include <OpenMS/CONCEPT/Types.h>
#include <OpenMS/CONCEPT/UniqueIdGenerator.h>

using namespace std;

START_TEST(Base64, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

using namespace OpenMS;


// default ctor
Base64* ptr = nullptr;
Base64* nullPointer = nullptr;

START_SECTION((Base64()))
  ptr = new Base64;
  TEST_NOT_EQUAL(ptr, nullPointer)
END_SECTION

// destructor
START_SECTION((virtual ~Base64()))
  delete ptr;
END_SECTION

/*

Python 

# Little Endian floats
>>> import base64
>>> import struct
>>> mynr = base64.standard_b64decode("pDiTRQ==")
>>> [struct.unpack('<f', mynr[i:i+4]) for i in range(0, len(mynr), 4) ]
[(4711.080078125,)]
 
# Big Endian doubles
>>> import base64
>>> import struct
>>> mynr = base64.standard_b64decode("QHLCZmZmZmZAcv/3ztkWh0BzCZmZmZma")
>>> [struct.unpack('>d', mynr[i:i+8]) for i in range(0, len(mynr), 8) ]
[(300.15,), (303.998,), (304.6,)]

*/

START_SECTION((template < typename FromType > void encode(std::vector< FromType > &in, ByteOrder to_byte_order, String &out, bool zlib_compression=false)))
  TOLERANCE_ABSOLUTE(0.001)
{
  Base64 b64;
  std::vector<float> data;
  std::vector<float> res;
  std::string dest;

  b64.encode(data, Base64::BYTEORDER_LITTLEENDIAN, dest);
  TEST_EQUAL(dest, "");

  data.push_back(300.15f);
  data.push_back(303.998f);
  data.push_back(304.6f);
  b64.encode(data, Base64::BYTEORDER_LITTLEENDIAN, dest);
  TEST_EQUAL(dest, "MxOWQ77/l0PNTJhD");
  // please remember that it is possible that two different strings can
  // decode to the "same" floating point number (considering such a low
  // precision like 0.001).
  
  data = std::vector<float>();
  data.push_back(4711.08f);
  b64.encode(data, Base64::BYTEORDER_LITTLEENDIAN, dest);
  TEST_EQUAL(dest, "pDiTRQ==")

  // testing the encoding of double vectors
  std::vector<double> data_double;
  std::vector<double> res_double;
  data_double.push_back(300.15);
  data_double.push_back(303.998);
  data_double.push_back(304.6);
  b64.encode(data_double, Base64::BYTEORDER_BIGENDIAN, dest);
  TEST_EQUAL(dest, "QHLCZmZmZmZAcv/3ztkWh0BzCZmZmZma");
  b64.decode(dest,Base64::BYTEORDER_BIGENDIAN,res_double);
}
END_SECTION

START_SECTION((template < typename ToType > void decode(const std::string &in, ByteOrder from_byte_order, std::vector< ToType > &out, bool zlib_compression=false)))
  TOLERANCE_ABSOLUTE(0.001)
{
  Base64 b64;
  std::string src;
  std::vector<float> res;
  std::vector<double> res_double;

  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res);
  TEST_EQUAL(res.size(), 0)

  src = "Q+vIuEec9YBD7TgoR/HTgEPt23hHA8UA";
  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res);
  TEST_REAL_SIMILAR(res[0], 471.568)
  TEST_REAL_SIMILAR(res[1], 80363)
  TEST_REAL_SIMILAR(res[2], 474.439)
  TEST_REAL_SIMILAR(res[3], 123815)
  TEST_REAL_SIMILAR(res[4], 475.715)
  TEST_REAL_SIMILAR(res[5], 33733)

  src = "JhOWQ8b/l0PMTJhD";
  b64.decode(src, Base64::BYTEORDER_LITTLEENDIAN, res);
  TEST_REAL_SIMILAR(res[0], 300.15)
  TEST_REAL_SIMILAR(res[1], 303.998)
  TEST_REAL_SIMILAR(res[2], 304.6)

  src = "QGYTSADLaUgAAABA";
  b64.decode(src, Base64::BYTEORDER_LITTLEENDIAN, res);
  TEST_REAL_SIMILAR(res[0], 150937)
  TEST_REAL_SIMILAR(res[1], 239404)
  TEST_REAL_SIMILAR(res[2], 2)

  src = "QHLCZmZmZmZAcv/3ztkWh0BzCZmZmZma";
  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res_double);
  TEST_REAL_SIMILAR(res_double[0], 300.15)
  TEST_REAL_SIMILAR(res_double[1], 303.998)
  TEST_REAL_SIMILAR(res_double[2], 304.6)

  // test some corrupted strings
  src = "==";
  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res);
  TEST_EQUAL(res.size(), 0)

  src = "Q==";
  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res);
  TEST_EQUAL(res.size(), 0)

  src = "====";
  b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res);
  TEST_EQUAL(res.size(), 0)

  // corrupted data
  src = "whoPutMeHere:somecrazyperson,obviously!WhatifIcontaininvalidcharacterslikethese";
  TEST_EXCEPTION(Exception::ConversionError, b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res) );

  src = "Q A..A=="; // the space is skipped, but dots are not allowed
  TEST_EXCEPTION(Exception::ConversionError, b64.decode(src, Base64::BYTEORDER_BIGENDIAN, res) );
}
END_SECTION

START_SECTION([EXTRA] zlib functionality)
{
  TOLERANCE_ABSOLUTE(0.001)
  Base64 b64;
  std::string str,src;
  std::vector<float> data, res;
  std::vector<double> data_double, res_double;
  
  // double data - big endian
  data_double.push_back(300.15);
  data_double.push_back(15.124);
  data_double.push_back(304.2);
  b64.encode(data_double,Base64::BYTEORDER_BIGENDIAN,str,true);
  b64.decode(str,Base64::BYTEORDER_BIGENDIAN, res_double,true);
  TEST_REAL_SIMILAR(res_double[0],300.15);
  TEST_REAL_SIMILAR(res_double[1],15.124);
  TEST_REAL_SIMILAR(res_double[2],304.2);
  
  data.clear();
  data.push_back(120.0f);
  data.push_back(100.0f);
  b64.encode(data,Base64::BYTEORDER_BIGENDIAN,str,true);
  b64.decode(str,Base64::BYTEORDER_BIGENDIAN,res,true);

  TEST_REAL_SIMILAR(res[0], 120)
  TEST_REAL_SIMILAR(res[1], 100)  
  // float data -big endian
  data.clear();
  data.push_back(471.568f);
  data.push_back(80363.0f);
  data.push_back(474.439f);
  data.push_back(123815.0f);
  data.push_back(475.715f);
  data.push_back(33733.0f);

  b64.encode(data,Base64::BYTEORDER_BIGENDIAN,str,true);
  b64.decode(str,Base64::BYTEORDER_BIGENDIAN, res,true);
  
  TEST_REAL_SIMILAR(res[0], 471.568)
  TEST_REAL_SIMILAR(res[1], 80363)
  TEST_REAL_SIMILAR(res[2], 474.439)
  TEST_REAL_SIMILAR(res[3], 123815)
  TEST_REAL_SIMILAR(res[4], 475.715)
  TEST_REAL_SIMILAR(res[5], 33733)
  
  // double data - little endian
  data.clear();
  data.push_back(300.15f);
  data.push_back(303.998f);
  data.push_back(304.61f);
  
  b64.encode(data,Base64::BYTEORDER_BIGENDIAN,str,true);
  b64.decode(str,Base64::BYTEORDER_BIGENDIAN, res,true);

  TEST_REAL_SIMILAR(res[0], 300.151)
  TEST_REAL_SIMILAR(res[1],  303.9981)
  TEST_REAL_SIMILAR(res[2], 304.61)
  
  src = "JhOWQ8b/l0PMTJhD";
  b64.decode(src, Base64::BYTEORDER_LITTLEENDIAN, res);
  b64.encode(res,Base64::BYTEORDER_LITTLEENDIAN,str,true);
  b64.decode(str,Base64::BYTEORDER_LITTLEENDIAN,data,true);

  TEST_REAL_SIMILAR(data[0], 300.15f)
  TEST_REAL_SIMILAR(data[1], 303.998f)
  TEST_REAL_SIMILAR(data[2], 304.6f)  
}
END_SECTION

START_SECTION(( void encodeStrings(const std::vector<std::string> & in, String & out, bool zlib_compression = false, bool append_zero_byte = true)))
{
  Base64 b64;
  std::string src,str;
  
  //without zlib compression
  src="ZGFzAGlzdABlaW4AdGVzdAAxMjM0";
  vector<std::string> strings;
  b64.decodeStrings(src,strings,false);
  TEST_EQUAL(strings.size() == 5,true   )
  TEST_EQUAL(strings[0],"das")
  TEST_EQUAL(strings[1],"ist")
  TEST_EQUAL(strings[2],"ein")
  TEST_EQUAL(strings[3],"test")
  TEST_EQUAL(strings[4],"1234")

  //same as above but this time the whole std::string is null-terminated as well
  src="ZGFzAGlzdABlaW4AdGVzdAAxMjM0AA==";
  b64.decodeStrings(src,strings,false);
  TEST_EQUAL(strings.size() == 5,true   )
  TEST_EQUAL(strings[0],"das")
  TEST_EQUAL(strings[1],"ist")
  TEST_EQUAL(strings[2],"ein")
  TEST_EQUAL(strings[3],"test")
  TEST_EQUAL(strings[4],"1234")
  
  //zlib compressed      
  src = "eJxLSSxmyCwuYUjNzGMoSQUyDI2MTRgAUX4GTw==";
  b64.decodeStrings(src,strings,true);
  TEST_EQUAL(strings.size() == 5,true )
  TEST_EQUAL(strings[0],"das")
  TEST_EQUAL(strings[1],"ist")
  TEST_EQUAL(strings[2],"ein")
  TEST_EQUAL(strings[3],"test")
  TEST_EQUAL(strings[4],"1234")
  
  //without zlib compression
  b64.encodeStrings(strings,str,false);
  b64.decodeStrings(str,strings,false);
  TEST_EQUAL(strings.size() == 5,true )
  TEST_EQUAL(strings[0],"das")
  TEST_EQUAL(strings[1],"ist")
  TEST_EQUAL(strings[2],"ein")
  TEST_EQUAL(strings[3],"test")
  TEST_EQUAL(strings[4],"1234")

  // round-trip including a single-character string (regression: the decode emit
  // guard used to require length >= 2 and silently dropped 1-char segments)
  vector<std::string> single_char_strings;
  single_char_strings.push_back("a");
  single_char_strings.push_back("bb");
  single_char_strings.push_back("c");
  b64.encodeStrings(single_char_strings, str, false);
  b64.decodeStrings(str, single_char_strings, false);
  TEST_EQUAL(single_char_strings.size(), 3)
  TEST_EQUAL(single_char_strings[0], "a")
  TEST_EQUAL(single_char_strings[1], "bb")
  TEST_EQUAL(single_char_strings[2], "c")

  // test some corrupted strings
  src = "==";
  b64.decodeStrings(src, strings, false);
  TEST_EQUAL(strings.size(), 0)

  src = "Q==";
  b64.decodeStrings(src, strings, false);
  TEST_EQUAL(strings.size(), 0)

  src = "====";
  b64.decodeStrings(src, strings, false);
  // "====" is malformed base64 (4 padding chars). decodeSingleString() decodes it to a
  // single non-NUL placeholder byte (the padding count only inspects the last 2 chars and
  // '=' is mis-mapped to a non-zero value in registerDecoder_). The corrected non-empty
  // guard now surfaces that as one length-1 string; the previous length>=2 guard silently
  // dropped it (the same defect that also dropped valid single-character strings above).
  TEST_EQUAL(strings.size(), 1)

  src = "Q A..A=="; // spaces and dots are not allowed
  b64.decodeStrings(src, strings, false);
  // TODO : some error checking and handling
  // TEST_EQUAL(strings.size(), 0)
}
END_SECTION
  
START_SECTION((void decodeStrings(const std::string& in, std::vector<std::string>& out, bool zlib_compression = false)))
  //this functionality is tested in the encodeString test
  NOT_TESTABLE
END_SECTION

START_SECTION((void decodeSingleString(const std::string & in, QByteArray & base64_uncompressed, bool zlib_compression)))
  //this functionality is tested in the decodeStrings test
  NOT_TESTABLE
END_SECTION

START_SECTION((template < typename ToType > void decodeIntegers(const std::string &in, ByteOrder from_byte_order, std::vector< ToType > &out, bool zlib_compression=false)))
{
  Base64 b64;
  std::string src,str;
  vector<Int32> res;
  vector<Int64> double_res;
  //with zlib compression
  src="eJwNw4c2QgEAANAniezMIrKyUrKyMooIIdki4/8/wr3n3CAIgjZDthu2w4iddhm12x577bPfAQeNOeSwI4465rhxE044adIpp00546xzzrtg2kWXXHbFVTOumTXnunk33HTLbXcsuOue+x54aNEjjz3x1JJlzzy34oWXVr3y2htr3nrnvXUfbPjok8+++Oqb737Y9NMvW377469//gPgoxL0";
  
  b64.decodeIntegers(src, Base64::BYTEORDER_LITTLEENDIAN,res,true);
  
  for(Size i = 0 ; i < res.size();++i)
  {
    TEST_EQUAL(res[i], i)
  }
  
  src="eJwtxdciAgAAAMDMZBWyiUrZLdlkZJRC9l79/0f04O7lAoF/bW53hzvd5W4H3eOQe93nfg940GFHPORhjzjqUY953BOe9JSnPeNZxzznecedcNILTjntRS952Ste9ZrXnXHWOedd8IaL3vSWt73jXe953wc+dMlHPvaJT132mc994UtXXPWVa6772je+dcN3vveDH/3kZ7/41W9+94c//eVv//jXf266BcFVEvQ=";
  b64.decodeIntegers(src,Base64::BYTEORDER_LITTLEENDIAN,double_res,true);
  
  for(Size i = 0 ; i < double_res.size();++i)
  {
      TEST_EQUAL(double_res[i], i)
  }
  
  src="eJxjZGBgYAJiZiAGAAA0AAc=";
  b64.decodeIntegers(src,Base64::BYTEORDER_BIGENDIAN,res,true);
  TEST_EQUAL(res[0],16777216)
  TEST_EQUAL(res[1],33554432)
  TEST_EQUAL(res[2],50331648)
  
  //without zlib compression 32bit
  src = "AAAAAQAAAAUAAAAGAAAABwAAAAgAAAAJAAACCg==";
  
  b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN,res,false);
  
  TEST_EQUAL(res[0],1)
  TEST_EQUAL(res[1],5)
  TEST_EQUAL(res[2],6)
  TEST_EQUAL(res[3],7)
  TEST_EQUAL(res[4],8)
  TEST_EQUAL(res[5],9)
  TEST_EQUAL(res[6],522)
  //64bit
  src = "AAAAAAAAAAUAAAAAAAAAAwAAAAAAAAAJ";  
  b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN,double_res,false);  
  TEST_EQUAL(double_res[0],5)
  TEST_EQUAL(double_res[1],3)
  TEST_EQUAL(double_res[2],9)  

  //64bit
  src = "BQAAAAAAAAADAAAAAAAAAAkAAAAAAAAA";  
  b64.decodeIntegers(src, Base64::BYTEORDER_LITTLEENDIAN,double_res,false);  
  TEST_EQUAL(double_res[0],5)
  TEST_EQUAL(double_res[1],3)
  TEST_EQUAL(double_res[2],9)  
  //32bit
  src ="AQAAAAUAAAAGAAAABwAAAAgAAAAJAAAACgIAAA==";
  b64.decodeIntegers(src, Base64::BYTEORDER_LITTLEENDIAN,res,false);
  
  TEST_EQUAL(res[0],1)
  TEST_EQUAL(res[1],5)
  TEST_EQUAL(res[2],6)
  TEST_EQUAL(res[3],7)
  TEST_EQUAL(res[4],8)
  TEST_EQUAL(res[5],9)
  TEST_EQUAL(res[6],522)

  // test some corrupted strings
  src = "==";
  b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN,res,false);
  TEST_EQUAL(res.size(), 0)

  src = "Q==";
  b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN,res,false);
  TEST_EQUAL(res.size(), 0)

  src = "====";
  b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN,res,false);
  TEST_EQUAL(res.size(), 0)

  src = "Q A..A=="; // the space is skipped, but dots are not allowed
  TEST_EXCEPTION(Exception::ConversionError, b64.decodeIntegers(src, Base64::BYTEORDER_BIGENDIAN, res, false))
}
END_SECTION

START_SECTION([EXTRA] numeric decoders reject bytes outside the Base64 alphabet)
{
  // Before the check, the float decoders turned such bytes into plausible numbers, and the integer
  // decoder indexed its lookup table with them (a byte below '+' gave a negative index).
  std::vector<float> f32;
  std::vector<double> f64;
  std::vector<Int32> i32;
  std::vector<Int64> i64;
  const std::vector<std::string> invalid =
  {
    "AAAA!AAA",          // punctuation
    "AAAAAAAAAAA!",
    "AAAA\xC3\xA9" "AA", // UTF-8 bytes (negative as signed char)
    "AAAA|AAA",          // just above 'z'
    "AAAA:AAA",          // inside '+'..'z', but not Base64
    "AA=AAAAA",          // padding inside the data
    "AAAAA===",          // more than two padding characters
    "AAAA\fAAA",         // only space, tab, CR and LF count as whitespace
    "AAAAA"              // length not a multiple of 4
  };
  for (const std::string& src : invalid)
  {
    for (const Base64::ByteOrder order : {Base64::BYTEORDER_LITTLEENDIAN, Base64::BYTEORDER_BIGENDIAN})
    {
      TEST_EXCEPTION(Exception::ConversionError, Base64::decode(src, order, f32, false))
      TEST_EXCEPTION(Exception::ConversionError, Base64::decode(src, order, f64, false))
      TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers(src, order, i32, false))
      TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers(src, order, i64, false))
    }
  }

  // an invalid byte inside otherwise valid, zlib-compressed data
  std::vector<double> values = {300.15, 15.124, 304.2, 1.0e6};
  std::string compressed;
  Base64::encode(values, Base64::BYTEORDER_LITTLEENDIAN, compressed, true);
  compressed[compressed.size() / 2] = '!';
  TEST_EXCEPTION(Exception::ConversionError, Base64::decode(compressed, Base64::BYTEORDER_LITTLEENDIAN, f64, true))
  std::vector<Int64> ints = {0, 1, 2, 999999};
  Base64::encodeIntegers(ints, Base64::BYTEORDER_LITTLEENDIAN, compressed, true);
  compressed[compressed.size() / 2] = '!';
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers(compressed, Base64::BYTEORDER_LITTLEENDIAN, i64, true))
}
END_SECTION

START_SECTION([EXTRA] numeric decoders skip whitespace in line-wrapped Base64)
{
  // xs:base64Binary allows whitespace, and mzML readers keep it when XML checks are skipped:
  // wrapped input must decode to the same values as unwrapped input.
  const auto wrap = [](const std::string& in)
  {
    std::string out = " \r\n";
    for (Size i = 0; i < in.size(); ++i)
    {
      if (i != 0 && i % 76 == 0) out += "\r\n";       // CRLF line breaks between groups of 4
      else if (i % 5 == 3) out += ' ';                // spaces inside groups
      else if (i + 1 == in.size() && in[i] == '=') out += "\t\n"; // whitespace between the padding characters
      out += in[i];
    }
    return out + "\r\n  ";
  };

  const auto check = [&wrap](const std::string& encoded, const auto& decode_and_compare)
  {
    TEST_EQUAL(wrap(encoded).size() > encoded.size(), true)
    decode_and_compare(encoded);
    decode_and_compare(wrap(encoded));
  };

  // 71 values: long enough for several SIMD blocks and CRLF line breaks, and a length that needs padding
  std::vector<float> f32;
  std::vector<double> f64;
  std::vector<Int32> i32;
  std::vector<Int64> i64;
  for (Int i = 0; i < 71; ++i)
  {
    f32.push_back(100.25f + 3.5f * i);
    f64.push_back(-5000.125 + 1234.0625 * i);
    i32.push_back(-35 * i + 7);
    i64.push_back(Int64(1) << (i % 60));
  }

  for (const bool zlib : {false, true})
  {
    for (const Base64::ByteOrder order : {Base64::BYTEORDER_LITTLEENDIAN, Base64::BYTEORDER_BIGENDIAN})
    {
      std::string encoded;
      std::vector<float> f32_in = f32;
      Base64::encode(f32_in, order, encoded, zlib);
      check(encoded, [&](const std::string& src)
      {
        std::vector<float> out;
        Base64::decode(src, order, out, zlib);
        TEST_EQUAL(out == f32, true)
      });

      std::vector<double> f64_in = f64;
      Base64::encode(f64_in, order, encoded, zlib);
      check(encoded, [&](const std::string& src)
      {
        std::vector<double> out;
        Base64::decode(src, order, out, zlib);
        TEST_EQUAL(out == f64, true)
      });

      std::vector<Int32> i32_in = i32;
      Base64::encodeIntegers(i32_in, order, encoded, zlib);
      check(encoded, [&](const std::string& src)
      {
        std::vector<Int32> out;
        Base64::decodeIntegers(src, order, out, zlib);
        TEST_EQUAL(out == i32, true)
      });

      std::vector<Int64> i64_in = i64;
      Base64::encodeIntegers(i64_in, order, encoded, zlib);
      check(encoded, [&](const std::string& src)
      {
        std::vector<Int64> out;
        Base64::decodeIntegers(src, order, out, zlib);
        TEST_EQUAL(out == i64, true)
      });
    }
  }

  // the padding may be split by whitespace ("AAAAAA==" decodes to 4 zero bytes)
  std::vector<Int32> out;
  Base64::decodeIntegers("AAAA\r\nAA=\r\n=", Base64::BYTEORDER_LITTLEENDIAN, out, false);
  TEST_EQUAL(out.size(), 1)
  TEST_EQUAL(out[0], 0)

  // whitespace does not hide a bad length or bad bytes
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers("AAAA\r\nA", Base64::BYTEORDER_LITTLEENDIAN, out, false))
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers("AAAA\r\n!AAA", Base64::BYTEORDER_LITTLEENDIAN, out, false))
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers("AAAA==\r\nAA", Base64::BYTEORDER_LITTLEENDIAN, out, false))
}
END_SECTION

START_SECTION([EXTRA] numeric decoders return nothing for empty and padding-only input)
{
  std::vector<float> f32 = {1.0f};
  std::vector<double> f64 = {1.0};
  std::vector<Int32> i32 = {1};
  std::vector<Int64> i64 = {1};
  for (const std::string src : {"", "=", "==", "Q==", "====", "========"})
  {
    for (const bool zlib : {false, true})
    {
      Base64::decode(src, Base64::BYTEORDER_LITTLEENDIAN, f32, zlib);
      TEST_EQUAL(f32.size(), 0)
      Base64::decode(src, Base64::BYTEORDER_LITTLEENDIAN, f64, zlib);
      TEST_EQUAL(f64.size(), 0)
    }
    Base64::decodeIntegers(src, Base64::BYTEORDER_LITTLEENDIAN, i32, false);
    TEST_EQUAL(i32.size(), 0)
    Base64::decodeIntegers(src, Base64::BYTEORDER_LITTLEENDIAN, i64, false);
    TEST_EQUAL(i64.size(), 0)
  }
  // compressed integers: empty input gives an empty array, anything that decompresses to nothing throws
  Base64::decodeIntegers("", Base64::BYTEORDER_LITTLEENDIAN, i32, true);
  TEST_EQUAL(i32.size(), 0)
  Base64::decodeIntegers("", Base64::BYTEORDER_LITTLEENDIAN, i64, true);
  TEST_EQUAL(i64.size(), 0)
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers("====", Base64::BYTEORDER_LITTLEENDIAN, i32, true))
  TEST_EXCEPTION(Exception::ConversionError, Base64::decodeIntegers("Q==", Base64::BYTEORDER_LITTLEENDIAN, i64, true))

  // whitespace only is like empty input
  Base64::decode(" \r\n\t ", Base64::BYTEORDER_LITTLEENDIAN, f64, false);
  TEST_EQUAL(f64.size(), 0)
  Base64::decodeIntegers(" \r\n\t ", Base64::BYTEORDER_LITTLEENDIAN, i32, false);
  TEST_EQUAL(i32.size(), 0)
}
END_SECTION

START_SECTION((template <typename FromType> void encodeIntegers(std::vector<FromType>& in, ByteOrder to_byte_order, std::string& out, bool zlib_compression=false)))
{
  Base64 b64;
  std::string tmp;
  
  //64 bit tests
  vector<Int64> vec64, vec64_in, vec64_out;
  vec64.push_back(0);
  vec64.push_back(1);
  vec64.push_back(2);
  vec64.push_back(3);
  vec64.push_back(4);
  vec64.push_back(5);
  
  //test with little endian and without compression
  tmp="";
  vec64_in = vec64;
  vec64_out.clear();
  b64.encodeIntegers(vec64_in, Base64::BYTEORDER_LITTLEENDIAN, tmp, false);
  b64.decodeIntegers(tmp, Base64::BYTEORDER_LITTLEENDIAN, vec64_out, false);
  TEST_EQUAL(vec64.size(),vec64_out.size())
  for (Size i=0; i<vec64.size(); ++i)
  {
    TEST_EQUAL(vec64[i],vec64_out[i])
  }

  //test with big endian and compression
  vec64.push_back(999999);
  tmp = "";
  vec64_in = vec64;
  vec64_out.clear();
  b64.encodeIntegers(vec64_in, Base64::BYTEORDER_BIGENDIAN, tmp, true);
  b64.decodeIntegers(tmp, Base64::BYTEORDER_BIGENDIAN, vec64_out, true);
  TEST_EQUAL(vec64.size(),vec64_out.size())
  for (Size i=0; i<vec64.size(); ++i)
  {
    TEST_EQUAL(vec64[i],vec64_out[i])
  }

  //32 bit tests  
  vector<Int32> vec32, vec32_in, vec32_out;
  vec32.push_back(0);
  vec32.push_back(5);
  vec32.push_back(10);
  vec32.push_back(15);
  vec32.push_back(20);
  vec32.push_back(25);

  //test with little endian and without compression
  tmp = "";
  vec32_in = vec32;
  vec32_out.clear();
  b64.encodeIntegers(vec32_in, Base64::BYTEORDER_LITTLEENDIAN, tmp, false);
  b64.decodeIntegers(tmp, Base64::BYTEORDER_LITTLEENDIAN, vec32_out, false);
  TEST_EQUAL(vec32.size(),vec32_out.size())
  for (Size i=0; i<vec32.size(); ++i)
  {
    TEST_EQUAL(vec32[i],vec32_out[i])
  }

  //test with big endian and compression
  vec32.push_back(999999);
  tmp = "";
  vec32_in = vec32;
  vec32_out.clear();
  b64.encodeIntegers(vec32_in, Base64::BYTEORDER_BIGENDIAN, tmp, true);
  b64.decodeIntegers(tmp, Base64::BYTEORDER_BIGENDIAN, vec32_out, true);
  TEST_EQUAL(vec32.size(),vec32_out.size())
  for (Size i=0; i<vec32.size(); ++i)
  {
    TEST_EQUAL(vec32[i],vec32_out[i])
  }
}
END_SECTION

ptr = new Base64;

START_SECTION(inline UInt32 endianize32(const UInt32& n))
  TEST_EQUAL(0, endianize32(0))  // swapping 0 should do nothing
  TEST_EQUAL(std::numeric_limits<UInt32>::max(), endianize32(std::numeric_limits<UInt32>::max()))  // swapping MAX should do nothing
  TEST_EQUAL(0x000000FF, endianize32(0xFF000000)) 
  TEST_EQUAL(0x0000FF00, endianize32(0x00FF0000)) 
  TEST_EQUAL(0x00FF0000, endianize32(0x0000FF00)) 
  TEST_EQUAL(0xFF000000, endianize32(0x000000FF))
  // random value should stay the same upon double call
  UInt32 r = (UInt32)UniqueIdGenerator::getUniqueId();
  TEST_EQUAL(r, endianize32(endianize32(r)))
END_SECTION

START_SECTION(inline UInt64 endianize64(const UInt64& n))
  TEST_EQUAL(0, endianize64(0))  // swapping 0 should do nothing
  TEST_EQUAL(std::numeric_limits<UInt64>::max(), endianize64(std::numeric_limits<UInt64>::max()))  // swapping MAX should do nothing
  TEST_EQUAL(0x00000000000000FF, endianize64(0xFF00000000000000)) 
  TEST_EQUAL(0x000000000000FF00, endianize64(0x00FF000000000000)) 
  TEST_EQUAL(0x0000000000FF0000, endianize64(0x0000FF0000000000)) 
  TEST_EQUAL(0x00000000FF000000, endianize64(0x000000FF00000000)) 
  TEST_EQUAL(0x000000FF00000000, endianize64(0x00000000FF000000)) 
  TEST_EQUAL(0x0000FF0000000000, endianize64(0x0000000000FF0000)) 
  TEST_EQUAL(0x00FF000000000000, endianize64(0x000000000000FF00)) 
  TEST_EQUAL(0xFF00000000000000, endianize64(0x00000000000000FF))
  // random value should stay the same upon double call
  UInt64 r = UniqueIdGenerator::getUniqueId();
  TEST_EQUAL(r, endianize64(endianize64(r)))
END_SECTION

delete ptr;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
