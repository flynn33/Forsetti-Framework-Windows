// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "ForsettiCore/SemVer.h"
#include <nlohmann/json.hpp>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;

TEST_CLASS(SemVerTests)
{
public:

    TEST_METHOD(DefaultConstruction)
    {
        SemVer v;
        Assert::AreEqual(0, v.major);
        Assert::AreEqual(0, v.minor);
        Assert::AreEqual(0, v.patch);
        Assert::IsFalse(v.prerelease.has_value());
    }

    TEST_METHOD(ComponentConstruction)
    {
        SemVer v(1, 2, 3);
        Assert::AreEqual(1, v.major);
        Assert::AreEqual(2, v.minor);
        Assert::AreEqual(3, v.patch);
        Assert::IsFalse(v.prerelease.has_value());
    }

    TEST_METHOD(PrereleaseConstruction)
    {
        SemVer v(1, 0, 0, "alpha");
        Assert::IsTrue(v.prerelease.has_value());
        Assert::AreEqual(std::string("alpha"), v.prerelease.value());
    }

    TEST_METHOD(FromString_ValidVersion)
    {
        auto v = SemVer::fromString("1.2.3");
        Assert::IsTrue(v.has_value());
        Assert::AreEqual(1, v->major);
        Assert::AreEqual(2, v->minor);
        Assert::AreEqual(3, v->patch);
        Assert::IsFalse(v->prerelease.has_value());
    }

    TEST_METHOD(FromString_WithPrerelease)
    {
        auto v = SemVer::fromString("1.0.0-beta.1");
        Assert::IsTrue(v.has_value());
        Assert::AreEqual(1, v->major);
        Assert::AreEqual(0, v->minor);
        Assert::AreEqual(0, v->patch);
        Assert::IsTrue(v->prerelease.has_value());
        Assert::AreEqual(std::string("beta.1"), v->prerelease.value());
    }

    TEST_METHOD(FromString_EmptyReturnsNullopt)
    {
        Assert::IsFalse(SemVer::fromString("").has_value());
    }

    TEST_METHOD(FromString_InvalidReturnsNullopt)
    {
        Assert::IsFalse(SemVer::fromString("abc").has_value());
        Assert::IsFalse(SemVer::fromString("1.2").has_value());
        Assert::IsFalse(SemVer::fromString("1").has_value());
    }

    TEST_METHOD(ToString_WithoutPrerelease)
    {
        SemVer v(1, 2, 3);
        Assert::AreEqual(std::string("1.2.3"), v.toString());
    }

    TEST_METHOD(ToString_WithPrerelease)
    {
        SemVer v(1, 0, 0, "alpha");
        Assert::AreEqual(std::string("1.0.0-alpha"), v.toString());
    }

    TEST_METHOD(Comparison_MajorVersion)
    {
        Assert::IsTrue(SemVer(1, 0, 0) < SemVer(2, 0, 0));
        Assert::IsTrue(SemVer(2, 0, 0) > SemVer(1, 0, 0));
    }

    TEST_METHOD(Comparison_MinorVersion)
    {
        Assert::IsTrue(SemVer(1, 1, 0) < SemVer(1, 2, 0));
    }

    TEST_METHOD(Comparison_PatchVersion)
    {
        Assert::IsTrue(SemVer(1, 1, 1) < SemVer(1, 1, 2));
    }

    TEST_METHOD(Comparison_EqualVersions)
    {
        Assert::IsTrue(SemVer(1, 0, 0) == SemVer(1, 0, 0));
    }

    TEST_METHOD(Comparison_PrereleaseIsLowerThanRelease)
    {
        // Per SemVer spec: release has higher precedence than prerelease
        Assert::IsTrue(SemVer(1, 0, 0, "alpha") < SemVer(1, 0, 0));
        Assert::IsTrue(SemVer(1, 0, 0) > SemVer(1, 0, 0, "alpha"));
    }

    TEST_METHOD(Comparison_PrereleaseOrdering)
    {
        Assert::IsTrue(SemVer(1, 0, 0, "alpha") < SemVer(1, 0, 0, "beta"));
    }

    TEST_METHOD(Comparison_PrereleaseSemVerPrecedence)
    {
        std::vector<SemVer> ordered = {
            SemVer(1, 0, 0, "alpha"),
            SemVer(1, 0, 0, "alpha.1"),
            SemVer(1, 0, 0, "alpha.beta"),
            SemVer(1, 0, 0, "beta"),
            SemVer(1, 0, 0, "beta.2"),
            SemVer(1, 0, 0, "beta.11"),
            SemVer(1, 0, 0, "rc.1"),
            SemVer(1, 0, 0)
        };

        for (std::size_t index = 1; index < ordered.size(); ++index) {
            Assert::IsTrue(ordered[index - 1] < ordered[index]);
        }
    }

    TEST_METHOD(Constructor_RejectsNegativeComponents)
    {
        Assert::ExpectException<std::invalid_argument>([]() {
            (void)SemVer(-1, 0, 0);
        });
    }

    TEST_METHOD(FromString_RejectsInvalidSemVerForms)
    {
        Assert::IsFalse(SemVer::fromString("+1.2.3").has_value());
        Assert::IsFalse(SemVer::fromString("1.2.3+build.1").has_value());
        Assert::IsFalse(SemVer::fromString("1.2.3-").has_value());
        Assert::IsFalse(SemVer::fromString("1.2.3-alpha..1").has_value());
        Assert::IsFalse(SemVer::fromString("1.2.3-01").has_value());
        Assert::IsFalse(SemVer::fromString("01.2.3").has_value());
        Assert::IsFalse(SemVer::fromString("1. 2.3").has_value());
    }

    TEST_METHOD(JSON_RoundTrip)
    {
        SemVer original(1, 2, 3, "rc.1");
        nlohmann::json j = original;
        SemVer restored = j.get<SemVer>();
        Assert::IsTrue(original == restored);
    }

    TEST_METHOD(JSON_NullPrerelease)
    {
        SemVer v(1, 0, 0);
        nlohmann::json j = v;
        Assert::IsTrue(j["prerelease"].is_null());
        SemVer restored = j.get<SemVer>();
        Assert::IsFalse(restored.prerelease.has_value());
    }

    TEST_METHOD(JSON_HasAllFields)
    {
        SemVer v(2, 5, 1);
        nlohmann::json j = v;
        Assert::AreEqual(2, j["major"].get<int>());
        Assert::AreEqual(5, j["minor"].get<int>());
        Assert::AreEqual(1, j["patch"].get<int>());
    }

    TEST_METHOD(JSON_RejectsInvalidVersion)
    {
        nlohmann::json j = {
            {"major", 1},
            {"minor", 0},
            {"patch", 0},
            {"prerelease", "alpha..1"}
        };

        Assert::ExpectException<std::invalid_argument>([&j]() {
            (void)j.get<SemVer>();
        });
    }
};
