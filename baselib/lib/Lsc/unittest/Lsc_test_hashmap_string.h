
#ifndef __BSL_TEST_HASHMAP_H
#define __BSL_TEST_HASHMAP_H

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <Lsc/containers/hash/Lsc_hashmap.h>
#include <Lsc/containers/hash/Lsc_phashmap.h>
#include <Lsc/containers/string/Lsc_string.h>
#include <Lsc/archive/Lsc_filestream.h>
#include <Lsc/archive/Lsc_serialization.h>
#include <Lsc/archive/Lsc_binarchive.h>
#include <Lsc/alloc/allocator.h>
#include <vector>
#include <map>

#define __XASSERT(flag, fmt, arg...)  \
{\
	bool ___Lsc_flag = flag; \
	if (!(___Lsc_flag)) { \
		fprintf(stdout, "\n[error][%s:%d][%s]"fmt"\n", __FILE__, __LINE__, #flag, ##arg); \
		return false; \
	}\
}

#define __XASSERT2(flag) __XASSERT(flag, "")

Lsc::string randstr()
{
	char buf[130] = {0};
	int len = rand()%64 + 1;
	for (int i=0; i<len; ++i) {
		buf[i] = rand()%26 + 'a';
	}
	return buf;
};

/**
 *key 的hash仿函数
 */
struct hash_key
{
	size_t operator () (const Lsc::string &__key) const {
		size_t key = 0;
		for (size_t i=0; i<__key.size(); ++i) {
			key = (key<<8)+__key[i];
		}
		return key;
	}
};


/*
 * hashmap测试
 */

template <typename THMp>
bool test_hashmap()
{
	{
		int loop = 1<<10;
		for (int i=0; i<loop; ++i) {
			THMp test_0;
		}
		for (int i=0; i<loop; ++i) {
			THMp test_0;
			test_0.create(10000);
		}
	}

	THMp test;
	//疯狂create
	for (int i=0; i<100; ++i) {
		__XASSERT2 (test.create(100) == 0);
	}
	//疯狂destroy
	for (int i=0; i<100; ++i) {
		__XASSERT2 (test.destroy() == 0);
	}

	__XASSERT2 (test.create(102400) == 0);

	//测试插入查询，是否正常
	int insertsiz = 1<<10;
	std::vector<Lsc::string> vec;
	for (int i=0; i<insertsiz; ++i) {
		Lsc::string str = randstr();
		int ret = test.set(str, str);
		__XASSERT (ret != -1, "set %s error", str.c_str());
		if (ret == Lsc::HASH_INSERT_SEC) {
			vec.push_back(str);
		}
	}

	int relinsertsiz = (int)vec.size();

	//是否能重复加入，在不覆盖下是否正常
	for (int i=0; i<relinsertsiz; ++i) {
		int ret = test.set(vec[i], vec[i]);
		__XASSERT( ret == Lsc::HASH_EXIST,
				"upece %s ret=%d", vec[i].c_str(), ret);
	}
	//重复加入覆盖下是否正常
	for (int i=0; i<relinsertsiz; ++i) {
		int ret = test.set(vec[i], vec[i], 1);
		__XASSERT( ret == Lsc::HASH_OVERWRITE,
				"overwrite err %s ret=%d", vec[i].c_str(), ret);
	}


	for (int i=0; i<relinsertsiz; ++i) {
		Lsc::string val="";
		int ret = test.get(vec[i], &val);
		__XASSERT (ret == Lsc::HASH_EXIST, "get %s err", vec[i].c_str());
		__XASSERT (vec[i] == val, "check get result err %s!=%s", 
				vec[i].c_str(), val.c_str());
	}

	//查询是否能成功删除
	for (int i=0; i<relinsertsiz; ++i) {
		int ret = test.erase(vec[i]);
		__XASSERT (ret == Lsc::HASH_EXIST, "erase %s no exist, ret=%d", vec[i].c_str(), ret);
		ret = test.get(vec[i]);
		__XASSERT (ret == Lsc::HASH_NOEXIST, "no erase %s", vec[i].c_str());
	}

	//再次插入
	for (int i=0; i<relinsertsiz; ++i) {
		int ret = test.set(vec[i], vec[i]);
		__XASSERT(ret == Lsc::HASH_INSERT_SEC, "insert %s error, ret=%d",
				vec[i].c_str(), ret);
	}

	//重复加入覆盖下是否正常
	for (int i=0; i<relinsertsiz; ++i) {
		int ret = test.set(vec[i], vec[relinsertsiz - i - 1], 1);
		__XASSERT( ret == Lsc::HASH_OVERWRITE,
				"overwrite err %s ret=%d", vec[i].c_str(), ret);
	}

	//查询是否正常
	for (int i=0; i<relinsertsiz; ++i) {
		Lsc::string val = "";
		int ret = test.get(vec[i], &val);
		__XASSERT( ret == Lsc::HASH_EXIST, "hash %s no exist ret=%d",
				vec[i].c_str(), ret);
		__XASSERT(val == vec[relinsertsiz-i-1], "%s != %s",
				val.c_str(), vec[relinsertsiz-i-1].c_str());
	}

	return test.destroy() == 0;
}

template <typename THMp>
bool test_hashmap_iterator()
{
	THMp test;
	std::map<Lsc::string, Lsc::string> vec;
	__XASSERT2 (test.create(1<<15) == 0);

	int insertsize = 1<<10;
	for (int i=0; i<insertsize; ++i) {
		Lsc::string key = randstr();
		Lsc::string val = randstr();
		int ret = test.set(key, val, 1);
		__XASSERT2 (ret != -1);
		vec[key] = val;
	}

	__XASSERT(test.size() == vec.size(), "size unequl %lu != %lu",
			(unsigned long)test.size(), (unsigned long)vec.size());

	for (typename THMp::iterator iter = test.begin(); iter != test.end(); ++iter) {
		__XASSERT2 (vec.find(iter->first)->second == iter->second);
	}

	const THMp & testp = test;
	for (typename THMp::const_iterator iter = testp.begin(); iter != testp.end(); ++iter) {
		__XASSERT2 (vec.find(iter->first)->second == iter->second);
	}
	testp.get(randstr());

	size_t vsize = vec.size();
	//根据序列化参数对value进行附值
	for (typename THMp::iterator iter = test.begin(); iter != test.end(); ++iter) {
		iter->second = randstr();
		vec[iter->first] = iter->second;
	}

	__XASSERT(vsize == vec.size(), "%lu != %lu", (unsigned long)vsize, (unsigned long)vec.size());

	for (typename THMp::iterator iter = test.begin(); iter != test.end(); ++iter) {
		__XASSERT2 (vec.find(iter->first)->second == iter->second);
	}

	//不调用destroy，自己调用析构函数析构
	//需要valgrind检查是否内存泄露
	return true;
}

template <class THMp>
bool test_hashmap_serialization()
{
	THMp test;
	std::map<Lsc::string, Lsc::string> vec;
	__XASSERT2 (test.create(1<<15) == 0);
	int loop = 1<<12;
	for (int i=0; i<loop; ++i) {
		Lsc::string key = randstr();
		Lsc::string val = randstr();
		__XASSERT2 (test.set(key, val, 1) != -1);
		vec[key] = val;
	}

	const char * htdat = "hashtest.data";
	//写硬盘
	{
		Lsc::filestream fs;
		__XASSERT2 (fs.open(htdat, "w") == 0);
		Lsc::binarchive ar(fs);
		__XASSERT2 (ar.write(test) == 0);
		fs.close();
	}
	//读硬盘
	{
		Lsc::filestream fs;
		__XASSERT2 (fs.open(htdat, "r") == 0);
		Lsc::binarchive ar(fs);
		__XASSERT2 (ar.read(&test) == 0);
		fs.close();

		//check
		__XASSERT (vec.size() == test.size(), "%lu != %lu",
				(unsigned long)vec.size(), (unsigned long)test.size());
		for (std::map<Lsc::string, Lsc::string>::iterator iter = vec.begin();
				iter != vec.end(); ++iter) {
			Lsc::string val = "";
			int ret = test.get(iter->first, &val);
			__XASSERT2 (ret == Lsc::HASH_EXIST);
			__XASSERT2 (val == iter->second);
		}
	}
	test.clear();
	//读硬盘到新结构体
	{
		THMp test_0;
		__XASSERT2 (test_0.create(100) == 0);
		Lsc::filestream fs;
		__XASSERT2 (fs.open(htdat, "r") == 0);
		Lsc::binarchive ar(fs);
		__XASSERT2 (ar.read(&test_0) == 0);
		fs.close();

		//check
		__XASSERT (vec.size() == test_0.size(), "%lu != %lu",
				(unsigned long)vec.size(), (unsigned long)test_0.size());
		for (std::map<Lsc::string, Lsc::string>::iterator iter = vec.begin();
				iter != vec.end(); ++iter) {
			Lsc::string val = "";
			int ret = test_0.get(iter->first, &val);
			__XASSERT2 (ret == Lsc::HASH_EXIST);
			__XASSERT2 (val == iter->second);
		}
	}
	//读硬盘到新结构体, 新结构体不初始化
	{
		THMp test_0;
		Lsc::filestream fs;
		__XASSERT2 (fs.open(htdat, "r") == 0);
		Lsc::binarchive ar(fs);
		__XASSERT2 (ar.read(&test_0) == 0);
		fs.close();

		//check
		__XASSERT (vec.size() == test_0.size(), "%lu != %lu",
				(unsigned long)vec.size(), (unsigned long)test_0.size());
		for (std::map<Lsc::string, Lsc::string>::iterator iter = vec.begin();
				iter != vec.end(); ++iter) {
			Lsc::string val = "";
			int ret = test_0.get(iter->first, &val);
			__XASSERT2 (ret == Lsc::HASH_EXIST);
			__XASSERT2 (val == iter->second);
		}
	}
	return true;
}

//assign测试
template <class THMp>
bool test_hashmap_assign()
{
	THMp test;
	std::map<Lsc::string, Lsc::string> vec;
	__XASSERT2 (test.create(102400) == 0);
	__BSL_DEBUG("create hashmap");
	int loop = 1<<15;
	for (int i=0; i<loop; ++i) {
		vec.insert(std::make_pair(randstr(), randstr()));
	}
	__BSL_DEBUG("insert success");
	__XASSERT2 (test.assign(vec.begin(), vec.end()) == 0);
	__BSL_DEBUG("assign success");

	for (std::map<Lsc::string, Lsc::string>::iterator iter = vec.begin();
			iter != vec.end(); ++iter) {
		Lsc::string val = "";
		int ret = test.get(iter->first, &val);
		__XASSERT2 (ret == Lsc::HASH_EXIST);
		__XASSERT (val == iter->second, "%s == %s", val.c_str(), iter->second.c_str());
	}

	for (int i=0; i<16; ++i) {
		__XASSERT2 (test.assign(vec.begin(), vec.end()) == 0);
	}
	return true;
}

template <typename THMp>
bool test_hashmap_clear()
{
	//没有create的情况下clear
	{
		THMp test0;
		__XASSERT2(test0.size() == 0);
		for (int i=0; i<10000; ++i) {
			__XASSERT2(test0.clear() == 0);
			__XASSERT2(test0.size() == 0);
		}
	}
	//create的情况下clear
	{
		THMp test0;
		__XASSERT2(test0.size() == 0);
		__XASSERT2(test0.create(100000) == 0);
		__XASSERT2(test0.size() == 0);
		__XASSERT2(test0.clear() == 0);
		__XASSERT2(test0.size() == 0);
	}
	//有数据的情况下clear
	{
		THMp test0;
		typedef std::map<Lsc::string, Lsc::string> map_t;
		map_t vmap;
		for (int i=0; i<10000; ++i) {
			vmap.insert(std::make_pair(randstr(), randstr()));
		}
		__XASSERT2(test0.create(vmap.size() * 2) == 0);
		for (int i=0; i<10; ++i) {
			__XASSERT2(test0.assign(vmap.begin(), vmap.end()) == 0);
		}

		for (int i=0; i<10; ++i) {
			__XASSERT2(test0.clear() == 0);
			__XASSERT2(test0.size() == 0);
		}

		for (int i=0; i<100; ++i) {
			__XASSERT2(test0.destroy() == 0);
		}
	}
	return true;
}

class Lsc_test_hashmap : plclic CxxTest::TestSuite
{
plclic:
	typedef Lsc::hashmap<Lsc::string, Lsc::string, hash_key> THMp1;
	/**
	 *
	**/
	void test_hashmap_(void) {
		__BSL_DEBUG("open debug mode");
		TSM_ASSERT ("test_hashmap error", test_hashmap<THMp1>());
	}
	/**
	 *
	**/
	void test_hashmap_iterator_(void) {
		TSM_ASSERT ("", test_hashmap_iterator<THMp1>());
	}

	/**
	 *
	**/
	void test_hashmap_serialization_(void) {
		TSM_ASSERT ("", test_hashmap_serialization<THMp1>());
	}

	/**
	 *
	**/
	void test_hashmap_assign_() {
		TSM_ASSERT ("", test_hashmap_assign<THMp1>());
	}

	void test_hashmap_clear_() {
		TSM_ASSERT ("", test_hashmap_clear<THMp1>());
	}

	//Lsc_alloc分配器
	typedef Lsc::hashmap<Lsc::string, Lsc::string, hash_key, Lsc::equal<Lsc::string>, Lsc::Lsc_alloc<Lsc::string> > THMp2;
	void test_hashmap2_() {
		TSM_ASSERT ("", test_hashmap<THMp2>());
	}
	void test_hashmap2_iterator_() {
		TSM_ASSERT ("", test_hashmap_iterator<THMp2>());
	}
	void test_hashmap2_serialization_() {
		TSM_ASSERT ("", test_hashmap_serialization<THMp2>());
	}
	void test_hashmap2_assign_() {
		TSM_ASSERT("", test_hashmap_assign<THMp2>());
	}
	void test_hashmap2_clear_() {
		TSM_ASSERT ("", test_hashmap_clear<THMp2>());
	}

	//默认的phashmap
	typedef Lsc::phashmap<Lsc::string, Lsc::string, hash_key> THMp3;
	void test_phashmap_() {
		TSM_ASSERT ("", test_hashmap<THMp3>());
	}
	void test_phashmap_iterator_() {
		TSM_ASSERT ("", test_hashmap_iterator<THMp3>());
	}
	void test_phashmap_serialization_() {
		TSM_ASSERT ("", test_hashmap_serialization<THMp3>());
	}
	void test_phashmap_assign_() {
		TSM_ASSERT ("", test_hashmap_assign<THMp3>());
	}
	void test_phashmap_clear_() {
		TSM_ASSERT ("", test_hashmap_clear<THMp3>());
	}

	//Lsc_alloc的phashmap
	typedef Lsc::phashmap<Lsc::string, Lsc::string, hash_key, Lsc::equal<Lsc::string>, Lsc::Lsc_alloc<Lsc::string> > THMp4;
	void test_phashmap2_() {
		TSM_ASSERT ("", test_hashmap<THMp4>());
	}
	void test_phashmap2_iterator_() {
		TSM_ASSERT ("", test_hashmap_iterator<THMp4>());
	}
	void test_phashmap2_serialization_() {
		TSM_ASSERT ("", test_hashmap_serialization<THMp4>());
	}
	void test_phashmap2_assign_() {
		TSM_ASSERT ("", test_hashmap_assign<THMp4>());
	}
	void test_phashmap2_clear_() {
		TSM_ASSERT ("", test_hashmap_clear<THMp4>());
	}

	//Lsc_cpsalloc分配器
	typedef Lsc::phashmap<Lsc::string, Lsc::string, hash_key, Lsc::equal<Lsc::string>, Lsc::Lsc_cpsalloc<Lsc::Lsc_alloc<Lsc::string> > > THMp5;
	void test_phashmap3_() {
		TSM_ASSERT ("", test_hashmap<THMp5>());
	}
	void test_phashmp3_iterator_() {
		TSM_ASSERT ("", test_hashmap_iterator<THMp5>());
	}
	void test_phashmap3_serialization_() {
		TSM_ASSERT ("", test_hashmap_serialization<THMp5>());
	}
	void test_phashmap3_assign_() {
		TSM_ASSERT ("", test_hashmap_assign<THMp5>());
	}
	void test_phashmap3_clear_() {
		TSM_ASSERT ("", test_hashmap_clear<THMp5>());
	}
};

#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
