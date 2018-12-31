#include <Misc/Q3BspTypes.h>

namespace Misc
{

	void Q3Header::clear()
	{
		m_MagicWord[0] = m_MagicWord[1] = m_MagicWord[2] = m_MagicWord[3] = '0';
		m_Version = 0;
	}

	bool Q3Header::valid() const
	{
		return (
			m_MagicWord[0] == 'I' && m_MagicWord[1] == 'B' &&
			m_MagicWord[2] == 'S' && m_MagicWord[3] == 'P' &&
			m_Version == 0x2E
			);
	}


	TriangleList Q3BiQuadPatch::tesselate(const Q3Face& face, int faceId, int L )
	{
		TriangleList result;
		
		const auto L1 = L + 1;
		const auto step = 1.0f / static_cast<float>(L);
	
		std::vector<App::MeshVertex> vertices(L1 * L1);

		int i;
		for (i = 0; i <= TESS_LEVEL; ++i)
			vertices[i] = Math::evaluateQuadricPatch(m_controls[0], m_controls[3], m_controls[6], i * step);

		for (i = 1; i <= L; ++i)
		{
			Vertex tmp[3];
			int j;
			for (j = 0; j < 3; ++j)
			{
				auto k = 3 * j;
				tmp[j] = Math::evaluateQuadricPatch(m_controls[k + 0], m_controls[k + 1], m_controls[k + 2], i * step);
			}
			for (j = 0; j <= L; ++j)
				vertices[i * L1 + j] = Math::evaluateQuadricPatch(tmp[0], tmp[1], tmp[2], j * step);
		}

		std::vector<int> indices(L * L1 * 2);
		for (auto row = 0; row < L; ++row)
			for (auto col = 0; col <= L; ++col)
			{
				auto offset = row * L1 + col;
				indices[offset * 2 + 1] = row * L1 + col;
				indices[offset * 2 + 0] = (row + 1) * L1 + col;
			}

		auto triPerRow = L * 2;
		for (auto row = 0; row < L; ++row)
		{
			auto  startOffset = row * L1 * 2;
			for (auto verts = 0; verts < triPerRow; verts++)
			{
				Q3Triangle newTriangle(vertices[0].getNormal(), faceId, face.m_texIndex, face.m_lightmap);
			
				auto offset = startOffset + verts;
				for (auto j = 0; j < 3; ++j)
					newTriangle.m_vertices[j] = vertices[indices[offset + j]];

				if (verts & 0x01)
					std::swap(newTriangle.m_vertices[0], newTriangle.m_vertices[1]);
				std::reverse(std::begin(newTriangle.m_vertices), std::end(newTriangle.m_vertices));

				newTriangle.m_faceType = PATCHFACE;
				result.push_back(newTriangle);
			}
		}
		return result;
	}

	void Q3Patch::calcBounds()
	{
		m_bounds.clearBounds();
		//for (const auto& patch : m_patches)
		for (const auto& tris :m_triangles)
		for (const auto& verts : tris.m_vertices)
			m_bounds.updateBounds(verts.m_worldCoord);
	}

	void Q3Patch::smoothPatchNormals()
	{
		static const float EPS = 0.00125f;
		std::vector<Vertex*> verts;
		//for (auto& patch : m_patches)
		for (auto& tris : m_triangles)
		for (auto& vert : tris.m_vertices)
			verts.push_back(&vert);

		std::vector<Math::Vector3f> normals;		
		//todo speed up
		for (auto curVert : verts)
		{
			auto normal = curVert->getNormal();
			for (auto loopVert : verts)
				if (loopVert->m_worldCoord.distanceSquared( curVert->m_worldCoord) < EPS)
					normal += loopVert->getNormal();
			normals.push_back(normal.getNormalized());			
		}
		
		int count = 0;
		for (auto loopVert : verts)
			loopVert->setNormal(normals[count++]);
	}

	Misc::Q3Triangle::Q3Triangle()
		: m_normal( Math::Vector3f( 0.0f, 0.0f, 1.0f ) )
		, m_faceid( -1 )
		, m_shaderId(-1)
		, m_lightmapId(-1)
	{

	}

	Misc::Q3Triangle::Q3Triangle(const Math::Vector3f& normal, int faceId, int shaderId, int lightId) 
		: m_normal(normal)	
		, m_faceid( faceId )
		, m_shaderId(shaderId)
		, m_lightmapId(lightId)
	{

	}

	Math::BBox3f Misc::Q3Triangle::getBounds() const
	{
		Math::BBox3f result;
		result.updateBounds(m_vertices[0].m_worldCoord);
		result.updateBounds(m_vertices[1].m_worldCoord);
		result.updateBounds(m_vertices[2].m_worldCoord);
		return result;
	}

	float Misc::Q3Triangle::area() const
	{
		return Math::TriangleArea(
			m_vertices[0].m_worldCoord,
			m_vertices[1].m_worldCoord,
			m_vertices[2].m_worldCoord);
	}

	void Misc::Q3Triangle::calcNormal()
	{
		Math::Planef p( m_vertices[0].m_worldCoord, m_vertices[2].m_worldCoord, m_vertices[1].m_worldCoord);
		
		m_normal = p.m_normal;
	}

	void Q3LightMap::rescale(float factor)
	{
		for (int i = 0; i < 128 * 128 * 3; ++i)
		{
			auto val = (&m_lightmapData[0][0][0] + i);
			float curVal = static_cast<float>(*val) / 255.0f;

			curVal *= factor;
			curVal = Math::Clamp(0.0f, 1.0f, curVal);
			*val = static_cast<std::uint8_t>(curVal * 255.0f);
		}
	}

}