#include "../stdafx.h"
#include "CppUnitTest.h"
#include "OptixFunctionality.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DaisyRiotTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			Assert::AreEqual(OptixFunctionality::TriangleMath::calculateSurface(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)), 0.5f);
		}

	};
}