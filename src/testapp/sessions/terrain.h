/**
 * Open Space Program
 * Copyright © 2019-2023 Open Space Program Project
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include "../scenarios.h"
#include "planet-a/chunk_utils.h"

#include <osp/core/math_types.h>
#include <osp/drawing/draw_ent.h>

#include <planet-a/skeleton.h>
#include <planet-a/skeleton_subdiv.h>
#include <planet-a/chunk_generate.h>

namespace testapp::scenes
{

/**
 * @brief Scene orientation relative to planet center
 *
 * This is intended to be modified by a system responsible for handling floating origin and
 * the Scene's position within a universe.
 */
struct ACtxTerrainFrame
{
    osp::Vector3l       position;
    osp::Quaterniond    rotation;
    bool                active      {false};
};


struct ACtxTerrain
{
    // 'Skeleton' used for managing instances and relationships between vertices, triangles, and
    // chunks. These are all integer IDs that can be parented, connected, neighboring, etc...
    planeta::SubdivTriangleSkeleton     skeleton;
    planeta::SkeletonVertexData         skData;
    planeta::ChunkSkeleton              skChunks;

    planeta::ChunkMeshBufferInfo        chunkInfo;
    planeta::BasicTerrainGeometry       chunkGeom;

    planeta::ChunkScratchpad            chunkSP;
    planeta::SubdivScratchpad           scratchpad;
};

struct ACtxTerrainIco
{
    float   radius{};
    float   height{};

    std::array<planeta::SkVrtxId, 12>    icoVrtx;
    std::array<planeta::SkTriGroupId, 5> icoGroups;
    std::array<planeta::SkTriId, 20>     icoTri;
};

osp::Session setup_terrain(
        osp::TopTaskBuilder&        rBuilder,
        osp::ArrayView<entt::any>   topData,
        osp::Session const&         scene);

osp::Session setup_terrain_debug_draw(
        osp::TopTaskBuilder&        rBuilder,
        osp::ArrayView<entt::any>   topData,
        osp::Session const&         windowApp,
        osp::Session const&         sceneRenderer,
        osp::Session const&         cameraCtrl,
        osp::Session const&         commonScene,
        osp::Session const&         terrain,
        osp::draw::MaterialId       mat);


} // namespace testapp::scenes
