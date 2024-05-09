/**
 * Open Space Program
 * Copyright © 2019-2024 Open Space Program Project
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

/**
 * @file
 * @brief Provides types to assign vertex position and normal data to skeletons and chunk mesh
 */
#pragma once

#include "skeleton.h"
#include "chunk_utils.h"

#include <osp/core/math_int64.h>

namespace planeta
{

/**
 * @brief A subdividable triangle mesh intended as a structural frame for detailed terrain mesh
 *
 * Uses int64 coordinates capable of representing entire planets.
 *
 * Invariants must be followed in order to support seamless transitions between levels of detail:
 *
 * * Invariant A: Non-subdivided triangles can only neighbor ONE subdivided triangle.
 * * Invariant B: For each subdivided triangle neighboring a non-subdivided triangle, the
 *                subdivided triangle's two children neighboring the non-subdivided triangle
 *                must not be subdivided.
 *
 * This is intended for spherical planets, but can easily be used for flat terrain or other
 * weirder shapes.
 */
struct SkeletonVertexData
{
    osp::KeyedVec<planeta::SkVrtxId, osp::Vector3l> positions;
    osp::KeyedVec<planeta::SkVrtxId, osp::Vector3>  normals;
    osp::KeyedVec<planeta::SkTriId,  osp::Vector3l> centers;

    int scale{};
};

/**
 * @brief
 *
 * When a chunk is deleted, it needs subtract face normals of all of its deleted faces from all
 * connected shared vertices.
 */
struct FanNormalContrib
{
    SharedVrtxId shared;
    osp::Vector3 sum{osp::ZeroInit};
};

struct BasicTerrainGeometry
{
    void resize(ChunkSkeleton const& skCh, ChunkMeshBufferInfo const& info);

    std::vector<osp::Vector3>           chunkVbufPos;
    std::vector<osp::Vector3>           chunkVbufNrm;
    std::vector<osp::Vector3u>          chunkIbuf;

    /// 2D, each row is
    std::vector<planeta::FanNormalContrib>              chunkFanNormalContrib;

    /// parallel with skChunks.m_chunkSharedUsed
    std::vector<osp::Vector3>                           chunkFillSharedNormals;

    /// Non-normalized sum of face normals of connected faces
    osp::KeyedVec<planeta::SharedVrtxId, osp::Vector3>  sharedNormals;
};


struct TerrainFaceWriter
{
    // 'iterators' used by ArrayView
    using IndxIt_t      = osp::Vector3u*;
    using FanNormalIt_t = osp::Vector3*;
    using ContribIt_t   = FanNormalContrib*;

    void fill_add_face(VertexIdx a, VertexIdx b, VertexIdx c) noexcept
    {
        fan_add_face(a, b, c);
    }

    void fill_add_normal_shared(VertexIdx const vertex, ChunkLocalSharedId const local)
    {
        SharedVrtxId const shared = sharedUsed[local.value];

        fillNormalContrib[local.value]  += selectedFaceNormal;
        sharedNormals    [shared.value] += selectedFaceNormal;

        rSharedNormalsDirty.set(shared.value);
    }

    void fill_add_normal_filled(VertexIdx const vertex)
    {
        vbufNrm[vertex] += selectedFaceNormal;
    }

    void fan_add_face(VertexIdx a, VertexIdx b, VertexIdx c) noexcept
    {
        calculate_face_normal(a, b, c);

        selectedFaceIndx   = {a, b, c};
        *currentFace = selectedFaceIndx;
        std::advance(currentFace, 1);
    }

    void fan_add_normal_shared(VertexIdx const vertex, SharedVrtxId const shared)
    {
        sharedNormals[shared.value] += selectedFaceNormal;

        // Record contributions to shared vertex normal, since this needs to be subtracted when
        // the associated chunk is removed or restitched.

        // Find an existing FanNormalContrib for the given shared vertex.
        // Since each triangle added is in contact with the previous triangle added, we only need
        // to linear-search the previous few (4) contributions added. We also need to consider the
        // first few (4), since the last triangle added will loop around and touch the start,
        // forming a ring of triangles.
        bool found = false;
        FanNormalContrib &rContrib = [this, shared, &found] () -> FanNormalContrib&
        {
            auto const matches = [shared] (FanNormalContrib const& x) noexcept { return x.shared == shared; };

            ContribIt_t const searchAFirst = std::max<ContribIt_t>(std::prev(contribLast, 4), fanNormalContrib.begin());
            ContribIt_t const searchALast  = contribLast;
            ContribIt_t const searchBFirst = fanNormalContrib.begin();
            ContribIt_t const searchBLast  = std::min<ContribIt_t>(std::next(fanNormalContrib.begin(), 4), searchAFirst);

            if (ContribIt_t const foundTemp = std::find_if(searchAFirst, searchALast, matches);
                foundTemp != searchALast)
            {
                found = true;
                return *foundTemp;
            }

            if (ContribIt_t const foundTemp = std::find_if(searchBFirst, searchBLast, matches);
                foundTemp != searchBLast)
            {
                found = true;
                return *foundTemp;
            }

            LGRN_ASSERTM(std::none_of(fanNormalContrib.begin(), contribLast, matches), "search code above is broken XD");

            return *contribLast;
        }();

        if ( ! found )
        {
            rContrib.shared = shared;
            rContrib.sum = osp::Vector3{osp::ZeroInit};
            rSharedNormalsDirty.set(shared.value);
            std::advance(contribLast, 1);
            LGRN_ASSERT(contribLast != fanNormalContrib.end());
        }

        rContrib.sum += selectedFaceNormal;
    }

    void calculate_face_normal(VertexIdx a, VertexIdx b, VertexIdx c)
    {
        osp::Vector3 const u = vbufPos[b] - vbufPos[a];
        osp::Vector3 const v = vbufPos[c] - vbufPos[a];
        selectedFaceNormal = Magnum::Math::cross(u, v).normalized();
    }

    osp::ArrayView<osp::Vector3 const>  vbufPos;
    osp::ArrayView<osp::Vector3>        vbufNrm;
    osp::ArrayView<osp::Vector3>        sharedNormals;
    osp::ArrayView<osp::Vector3>        fillNormalContrib;;
    osp::ArrayView<FanNormalContrib>    fanNormalContrib;
    osp::ArrayView<SharedVrtxOwner_t>   sharedUsed;
    osp::Vector3                        selectedFaceNormal;
    osp::Vector3u                       selectedFaceIndx;
    IndxIt_t                            currentFace;
    ContribIt_t                         contribLast;
    osp::BitVector_t                    &rSharedNormalsDirty;
};
static_assert(CFaceWriter<TerrainFaceWriter>, "TerrainFaceWriter must satisfy concept CFaceWriter");



} // namespace planeta
