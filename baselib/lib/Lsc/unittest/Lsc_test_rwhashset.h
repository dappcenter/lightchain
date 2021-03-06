
#ifndef __BSL_TEST_HASHTABLE_H
#define __BSL_TEST_HASHTABLE_H

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <Lsc/containers/hash/Lsc_hashmap.h>
#include <Lsc/containers/string/Lsc_string.h>
#include <Lsc/containers/hash/Lsc_phashmap.h>
#include <Lsc/containers/hash/Lsc_hashtable.h>
#include <Lsc/containers/hash/Lsc_phashtable.h>

#include <Lsc/archive/Lsc_filestream.h>
#include <Lsc/archive/Lsc_serialization.h>
#include <Lsc/archive/Lsc_binarchive.h>
#include <Lsc/alloc/allocator.h>
#include <vector>
#include <map>
#include <set>

char s[][32]={"yingxiang", "yufan", "wangjiping", "xiaowei", "yujianjia", "maran", "baonenghui",
	"gonglei", "wangyue", "changxinglong", "chenxiaoming", "guoxingrong", "kuangyuheng"
};

const size_t N = sizeof(s) / sizeof(s[0]);


std::string randstr()
{
	char buf[130] = {0};
	int len = rand()%64 + 1;
	for (int i=0; i<len; ++i) {
		buf[i] = rand()%26 + 'a';
	}
	return buf;
};

/**
 *key ��hash�º���
 */
struct hash_func
{
	size_t operator () (const std::string &__key) const {
		size_t key = 0;
		for (size_t i=0; i<__key.size(); ++i) {
			key = (key<<8)+__key[i];
		}
		return key;
	}
};

class Lsc_test_rwhashset : plclic CxxTest::TestSuite
{
plclic:

	typedef std::string key;
	typedef int value;
	typedef Lsc::Lsc_rwhashset<key> hash_type;
	void test_rwhashset() {
		{
			hash_type ht;
			TS_ASSERT( 0 == ht.create(100));
		}
		{
			try {
				printf("hello\n");
				hash_type ht(0);
			} catch (Lsc::Exception& e) {
				printf("\n==\n%s\n==\n",e.all());
				printf("world\n");
			}
		}
		{
			hash_type ht(100);
			std::set<key> st;
			ht.assign(st.begin(),st.end());
		}		
	}
	void test_create() {
		hash_type ht;
		for (int i = 10; i < 100; i ++) {
			ht.create(i);
		}
		hash_type ht2;
		for (int i = 0; i < 100; i ++) {
			ht2.create(i);
		}
	}	
};

#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
