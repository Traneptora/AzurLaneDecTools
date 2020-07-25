#include <cstddef>
#include <cstdint>

struct Il2CppObject
{
	void *klass;
	void *monitor;
};

struct Il2CppArray : public Il2CppObject
{
	void *bounds;
	int32_t max_length;
};

struct FByteArray
{
	Il2CppObject obj = { 0 };
	void *bounds = nullptr;
	int32_t max_length = 0;
	char m_Items[65535];
};

Il2CppArray* NewSpecific(size_t element_size, size_t n);
FByteArray* DigitalSea_Scipio(FByteArray* _AssertArrayIn);