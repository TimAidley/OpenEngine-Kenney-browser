#include "KenneyPackName.h"

#include <gtest/gtest.h>

using GameEngine::KenneyAssets::KenneyPackPageUrl;
using GameEngine::KenneyAssets::ParseKenneyZipName;

TEST(KenneyPackName, ParsesSlugAndVersion)
{
    const auto name = ParseKenneyZipName("kenney_cube-pets_1.0.zip");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name->Slug, "cube-pets");
    EXPECT_EQ(name->Version, "1.0");
}

TEST(KenneyPackName, ParsesSlugWithoutVersion)
{
    const auto name = ParseKenneyZipName("kenney_3d-road-tiles.zip");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name->Slug, "3d-road-tiles");
    EXPECT_TRUE(name->Version.empty());
}

TEST(KenneyPackName, AcceptsBrowserDuplicateCounter)
{
    const auto name = ParseKenneyZipName("kenney_cube-pets_1.0 (2).zip");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name->Slug, "cube-pets");
    EXPECT_EQ(name->Version, "1.0");
}

TEST(KenneyPackName, RejectsPartialDownloads)
{
    EXPECT_FALSE(ParseKenneyZipName("kenney_cube-pets_1.0.zip.part"));
    EXPECT_FALSE(ParseKenneyZipName("kenney_cube-pets_1.0.zip.crdownload"));
    EXPECT_FALSE(ParseKenneyZipName("Unconfirmed 123456.crdownload"));
}

TEST(KenneyPackName, RejectsOtherZips)
{
    EXPECT_FALSE(ParseKenneyZipName("Kenney Game Assets All-in-1 3.7.0.zip"));
    EXPECT_FALSE(ParseKenneyZipName("kenney_.zip"));
    EXPECT_FALSE(ParseKenneyZipName("kenney_Cube Pets.zip"));
    EXPECT_FALSE(ParseKenneyZipName("kenney_cube-pets_beta.zip"));
    EXPECT_FALSE(ParseKenneyZipName("other_cube-pets.zip"));
}

TEST(KenneyPackName, PageUrlUsesSlug)
{
    EXPECT_EQ(KenneyPackPageUrl("cube-pets"), "https://kenney.nl/assets/cube-pets");
}
