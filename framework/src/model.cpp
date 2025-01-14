#include <gvk.hpp>
#include <sstream>

namespace gvk
{

	avk::owning_resource<model_t> model_t::load_from_file(const std::string& aPath, aiProcessFlagsType aAssimpFlags)
	{
		model_t result;
		result.mModelPath = avk::clean_up_path(aPath);
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFile(aPath, aAssimpFlags);
		if (nullptr == result.mScene) {
			throw gvk::runtime_error(fmt::format("Loading model from '{}' failed.", aPath));
		}
		result.initialize_materials();
		return result;
	}
	
	avk::owning_resource<model_t> model_t::load_from_memory(const std::string& aMemory, aiProcessFlagsType aAssimpFlags)
	{
		model_t result;
		result.mModelPath = "";
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFileFromMemory(aMemory.c_str(), aMemory.size(), aAssimpFlags);
		if (nullptr == result.mScene) {
			throw gvk::runtime_error("Loading model from memory failed.");
		}
		result.initialize_materials();
		return result;
	}

	
	void model_t::initialize_materials()
	{
		auto n = static_cast<size_t>(mScene->mNumMeshes);
		mMaterialConfigPerMesh.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			mMaterialConfigPerMesh.emplace_back();
		}
	}

	std::optional<glm::mat4> model_t::transformation_matrix_traverser(const unsigned int aMeshIndexToFind, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		for (unsigned int i = 0; i < aNode->mNumMeshes; i++)
		{
			if (aNode->mMeshes[i] == aMeshIndexToFind) {
				return glm::transpose(glm::make_mat4(&nodeM.a1));
			}
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser(aMeshIndexToFind, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	glm::mat4 model_t::transformation_matrix_for_mesh(mesh_index_t aMeshIndex) const
	{
		// Find the mesh in Assim's node hierarchy
		return transformation_matrix_traverser(static_cast<unsigned int>(aMeshIndex), mScene->mRootNode, aiMatrix4x4{}).value();
	}

	std::string model_t::name_of_mesh(mesh_index_t _MeshIndex) const
	{
		assert(mScene->mNumMeshes >= _MeshIndex);
		return mScene->mMeshes[_MeshIndex]->mName.data;
	}

	size_t model_t::material_index_for_mesh(mesh_index_t aMeshIndex) const
	{
		assert(mScene->mNumMeshes >= aMeshIndex);
		return mScene->mMeshes[aMeshIndex]->mMaterialIndex;
	}

	std::string model_t::name_of_material(size_t aMaterialIndex) const
	{
		aiMaterial* pMaterial = mScene->mMaterials[aMaterialIndex];
		if (!pMaterial) return "";
		aiString name;
		if (pMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			return name.data;
		else
			return "";
	}

	material_config model_t::material_config_for_mesh(mesh_index_t aMeshIndex)
	{
		assert (mMaterialConfigPerMesh.size() > aMeshIndex);
		if (mMaterialConfigPerMesh[aMeshIndex].has_value()) {
			return mMaterialConfigPerMesh[aMeshIndex].value();
		}
		
		material_config result;

		aiString strVal;
		aiColor3D color(0.0f, 0.0f, 0.0f);
		int intVal;
		aiBlendMode blendMode;
		float floatVal;
		aiTextureMapping texMapping;

		auto materialIndex = material_index_for_mesh(aMeshIndex);
		assert(materialIndex <= mScene->mNumMaterials);
		aiMaterial* aimat = mScene->mMaterials[materialIndex];

		// CPU-only parameters:
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, strVal)) {
			result.mName = strVal.data;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHADING_MODEL, intVal)) {
			switch (static_cast<aiShadingMode>(intVal)) {
			case aiShadingMode_Flat: 
				result.mShadingModel = "Flat";
				break;
			case aiShadingMode_Gouraud: 
				result.mShadingModel = "Gouraud";
				break;
			case aiShadingMode_Phong: 
				result.mShadingModel = "Phong";
				break;
			case aiShadingMode_Blinn: 
				result.mShadingModel = "Blinn";
				break;
			case aiShadingMode_Toon: 
				result.mShadingModel = "Toon";
				break;
			case aiShadingMode_OrenNayar: 
				result.mShadingModel = "OrenNayar";
				break;
			case aiShadingMode_Minnaert: 
				result.mShadingModel = "Minnaert";
				break;
			case aiShadingMode_CookTorrance: 
				result.mShadingModel = "CookTorrance";
				break;
			case aiShadingMode_NoShading: 
				result.mShadingModel = "NoShading";
				break;
			case aiShadingMode_Fresnel: 
				result.mShadingModel = "Fresnel";
				break;
			default: 
				result.mShadingModel = "";
			}
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_ENABLE_WIREFRAME, intVal)) {
			result.mWireframeMode = 0 != intVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_TWOSIDED, intVal)) {
			result.mTwosided = 0 != intVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_BLEND_FUNC, blendMode)) {
			result.mBlendMode = blendMode == aiBlendMode_Additive
				? avk::cfg::color_blending_config::enable_additive_for_all_attachments()
				: avk::cfg::color_blending_config::enable_alpha_blending_for_all_attachments();
		}

		// Shader parameters:
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
			result.mDiffuseReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
			result.mAmbientReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
			result.mSpecularReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
			result.mEmissiveColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_TRANSPARENT, color)) {
			result.mTransparentColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_REFLECTIVE, color)) {
			result.mReflectiveColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_OPACITY, floatVal)) {
			result.mOpacity = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_BUMPSCALING, floatVal)) {
			result.mBumpScaling = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS, floatVal)) {
			result.mShininess = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS_STRENGTH, floatVal)) {
			result.mShininessStrength = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFRACTI, floatVal)) {
			result.mRefractionIndex = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFLECTIVITY, floatVal)) {
			result.mReflectivity = floatVal;
		}

		// TODO: reading aiTextureMapMode (last parameter) corrupts stack for some .dae files (like "level01c.dae")
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mDiffuseTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SPECULAR, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mSpecularTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_AMBIENT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mAmbientTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_EMISSIVE, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mEmissiveTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_HEIGHT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mHeightTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mNormalsTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SHININESS, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mShininessTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_OPACITY, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mOpacityTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DISPLACEMENT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mDisplacementTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_REFLECTION, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mReflectionTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_LIGHTMAP, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, nullptr)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mLightmapTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
		}

		mMaterialConfigPerMesh[aMeshIndex] = result; // save
		return result;
	}

	void model_t::set_material_config_for_mesh(mesh_index_t aMeshIndex, const material_config& aMaterialConfig)
	{
		assert(aMeshIndex < mMaterialConfigPerMesh.size());
		mMaterialConfigPerMesh[aMeshIndex] = aMaterialConfig;
	}

	std::unordered_map<material_config, std::vector<size_t>> model_t::distinct_material_configs(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials)
	{
		assert(mScene);
		std::unordered_map<material_config, std::vector<size_t>> result;
		auto n = mScene->mNumMeshes;
		for (decltype(n) i = 0; i < n; ++i) {
			const aiMesh* paiMesh = mScene->mMeshes[i];
			auto matConf = material_config_for_mesh(i);
			matConf.mIgnoreCpuOnlyDataForEquality = !aAlsoConsiderCpuOnlyDataForDistinctMaterials;
			result[matConf].emplace_back(i);
		}
		return result;
	}

	size_t model_t::number_of_vertices_for_mesh(mesh_index_t aMeshIndex) const
	{
		assert(mScene);
		assert(aMeshIndex < mScene->mNumMeshes);
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		return static_cast<size_t>(paiMesh->mNumVertices);
	}

	std::vector<glm::vec3> model_t::positions_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(glm::vec3(paiMesh->mVertices[i][0], paiMesh->mVertices[i][1], paiMesh->mVertices[i][2]));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::normals_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		if (nullptr == paiMesh->mNormals) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain normals. Will return (0,0,1) normals for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0.f, 0.f, 1.f);
			}
		}
		else {
			// We've got normals. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mNormals[i][0], paiMesh->mNormals[i][1], paiMesh->mNormals[i][2]);
			}
		}
		return result;
	}

	std::vector<glm::vec3> model_t::tangents_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		if (nullptr == paiMesh->mTangents) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain tangents. Will return (1,0,0) tangents for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(1.f, 0.f, 0.f);
			}
		}
		else {
			// We've got tangents. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mTangents[i][0], paiMesh->mTangents[i][1], paiMesh->mTangents[i][2]);
			}
		}
		return result;
	}

	std::vector<glm::vec3> model_t::bitangents_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		if (nullptr == paiMesh->mBitangents) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain bitangents. Will return (0,1,0) bitangents for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0.f, 1.f, 0.f);
			}
		}
		else {
			// We've got bitangents. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mBitangents[i][0], paiMesh->mBitangents[i][1], paiMesh->mBitangents[i][2]);
			}
		}
		return result;
	}

	std::vector<glm::vec4> model_t::colors_for_mesh(mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec4> result;
		result.reserve(n);
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_COLOR_SETS);
		if (nullptr == paiMesh->mColors[aSet]) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain a color set at index {}. Will return opaque magenta for each vertex.", aMeshIndex, aSet));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(1.f, 0.f, 1.f, 1.f);
			}
		}
		else {
			// We've got colors[_Set]. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mColors[aSet][i][0], paiMesh->mColors[aSet][i][1], paiMesh->mColors[aSet][i][2], paiMesh->mColors[aSet][i][3]);
			}
		}
		return result;
	}

	std::vector<glm::vec4> model_t::bone_weights_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec4> result;
		result.reserve(n);
		if (!paiMesh->HasBones()) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain bone weights. Will return (1,0,0,0) bone weights for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(1.f, 0.f, 0.f, 0.f);
			}
		}
		else {
			// read bone indices and weights for bone animation
			std::vector<std::vector<std::tuple<uint32_t, float>>> vTempWeightsPerVertex(paiMesh->mNumVertices);
			for (unsigned int j = 0; j < paiMesh->mNumBones; j++) 
			{
				const aiBone * pBone = paiMesh->mBones[j];
				for (uint32_t b = 0; b < pBone->mNumWeights; b++) 
				{
					vTempWeightsPerVertex[pBone->mWeights[b].mVertexId].emplace_back(j, pBone->mWeights[b].mWeight);
				}
			}
			
			// We've got bone weights. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				auto& weights = result.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
				for (size_t j = 0; j < std::min(size_t{4}, vTempWeightsPerVertex[i].size()); ++j) {
					weights[j] = std::get<float>(vTempWeightsPerVertex[i][j]);
				}
			}
		}
		return result;
	}

	std::vector<glm::uvec4> model_t::bone_indices_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::uvec4> result;
		result.reserve(n);
		if (!paiMesh->HasBones()) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain bone weights. Will return (0,0,0,0) bone indices for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0u, 0u, 0u, 0u);
			}
		}
		else {
			// read bone indices and weights for bone animation
			std::vector<std::vector<std::tuple<uint32_t, float>>> vTempWeightsPerVertex(paiMesh->mNumVertices);
			for (unsigned int j = 0; j < paiMesh->mNumBones; j++) 
			{
				const aiBone * pBone = paiMesh->mBones[j];
				for (uint32_t b = 0; b < pBone->mNumWeights; b++) 
				{
					vTempWeightsPerVertex[pBone->mWeights[b].mVertexId].emplace_back(j, pBone->mWeights[b].mWeight);
				}
			}
			
			// We've got bone weights. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				auto& weights = result.emplace_back(0u, 0u, 0u, 0u);
				for (size_t j = 0; j < std::min(size_t{4}, vTempWeightsPerVertex[i].size()); ++j) {
					weights[j] = std::get<uint32_t>(vTempWeightsPerVertex[i][j]);
				}
			}
		}
		return result;
	}

	int model_t::num_uv_components_for_mesh(mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[aSet]) { return 0; }
		return paiMesh->mNumUVComponents[aSet];
	}

	int model_t::number_of_indices_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		size_t indicesCount = 0;
		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
		{
			const aiFace& paiFace = paiMesh->mFaces[i];
			indicesCount += paiFace.mNumIndices;
		}
		return static_cast<int>(indicesCount);
	}

	std::vector<size_t> model_t::select_all_meshes() const
	{
		std::vector<size_t> result;
		auto n = mScene->mNumMeshes;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(static_cast<size_t>(i));
		}
		return result;
	}

	std::optional<glm::mat4> model_t::transformation_matrix_traverser_for_light(const aiLight* aLight, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		if (aNode->mName == aLight->mName) {
			return glm::transpose(glm::make_mat4(&nodeM.a1));
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser_for_light(aLight, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	std::vector<glm::vec3> model_t::positions_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = positions_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::normals_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = normals_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::tangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = tangents_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::bitangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bitangents_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec4> model_t::colors_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet) const
	{
		std::vector<glm::vec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = colors_for_mesh(meshIndex, aSet);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec4> model_t::bone_weights_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bone_weights_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}
	
	std::vector<glm::uvec4> model_t::bone_indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::uvec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bone_indices_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<lightsource> model_t::lights() const
	{
		std::vector<lightsource> result;
		auto n = mScene->mNumLights;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			const aiLight* aiLight = mScene->mLights[i];
			glm::mat4 transfo = transformation_matrix_traverser_for_light(aiLight, mScene->mRootNode, aiMatrix4x4()).value();
			glm::mat3 transfoForDirections = glm::mat3(glm::inverse(glm::transpose(transfo))); // TODO: inverse transpose okay for direction??
			lightsource cgbLight;
			cgbLight.mAngleInnerCone = aiLight->mAngleInnerCone;
			cgbLight.mAngleOuterCone = aiLight->mAngleOuterCone;
			cgbLight.mAttenuationConstant = aiLight->mAttenuationConstant;
			cgbLight.mAttenuationLinear = aiLight->mAttenuationLinear;
			cgbLight.mAttenuationQuadratic = aiLight->mAttenuationQuadratic;
			auto aiColor = aiLight->mColorAmbient;
			cgbLight.mColorAmbient = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
			aiColor = aiLight->mColorDiffuse;
			cgbLight.mColor = glm::vec3(aiColor.r, aiColor.g, aiColor.b); // The "diffuse color" is considered to be the main color of this light source
			aiColor = aiLight->mColorSpecular;
			cgbLight.mColorSpecular = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
			auto aiVec = aiLight->mDirection;
			cgbLight.mDirection = transfoForDirections * glm::vec3(aiVec.x, aiVec.y, aiVec.z);
			cgbLight.mName = std::string(aiLight->mName.C_Str());
			aiVec = aiLight->mPosition;
			cgbLight.mPosition = transfo * glm::vec4(aiVec.x, aiVec.y, aiVec.z, 1.0f);
			aiVec = aiLight->mUp;
			cgbLight.mUpVector = transfoForDirections * glm::vec3(aiVec.x, aiVec.y, aiVec.z);
			cgbLight.mAreaExtent = glm::vec2(aiLight->mSize.x, aiLight->mSize.y);
			switch (aiLight->mType) {
			case aiLightSource_DIRECTIONAL:
				cgbLight.mType = lightsource_type::directional;
				break;
			case aiLightSource_POINT:
				cgbLight.mType = lightsource_type::point;
				break;
			case aiLightSource_SPOT:
				cgbLight.mType = lightsource_type::spot;
				break;
			case aiLightSource_AMBIENT:
				cgbLight.mType = lightsource_type::ambient;
			case aiLightSource_AREA:
				cgbLight.mType = lightsource_type::area;
			default:
				cgbLight.mType = lightsource_type::reserved0;
			}
			result.push_back(cgbLight);
		}
		return result;
	}

	std::vector<gvk::camera> model_t::cameras() const
	{
		std::vector<gvk::camera> result;
		result.reserve(mScene->mNumCameras);
		for (int i = 0; i < mScene->mNumCameras; ++i) {
			aiCamera* aiCam = mScene->mCameras[i];
			auto cgbCam = gvk::camera();
			cgbCam.set_aspect_ratio(aiCam->mAspect);
			cgbCam.set_far_plane_distance(aiCam->mClipPlaneFar);
			cgbCam.set_near_plane_distance(aiCam->mClipPlaneNear);
			cgbCam.set_field_of_view(aiCam->mHorizontalFOV);
			cgbCam.set_translation(glm::make_vec3(&aiCam->mPosition.x));
			glm::vec3 lookdir = glm::make_vec3(&aiCam->mLookAt.x);
			glm::vec3 updir = glm::make_vec3(&aiCam->mUp.x);
			cgbCam.set_rotation(glm::quatLookAt(lookdir, updir));
			aiMatrix4x4 projMat;
			aiCam->GetCameraMatrix(projMat);
			cgbCam.set_projection_matrix(glm::make_mat4(&projMat.a1));
			auto trafo = transformation_matrix_traverser_for_camera(aiCam, mScene->mRootNode, aiMatrix4x4());
			if (trafo.has_value()) {
				glm::vec3 side = glm::normalize(glm::cross(lookdir, updir));
				cgbCam.set_translation(trafo.value() * glm::vec4(cgbCam.translation(), 1));
				glm::mat3 dirtrafo = glm::mat3(glm::inverse(glm::transpose(trafo.value())));
				cgbCam.set_rotation(glm::quatLookAt(dirtrafo * lookdir, dirtrafo * updir));
			}
			result.push_back(cgbCam);
		}
		return result;
	}

	animation_clip_data model_t::load_animation_clip(unsigned int aAnimationIndex, double aStartTimeTicks, double aEndTimeTicks) const
	{
		assert(mScene);
		assert(aEndTimeTicks > aStartTimeTicks);
		assert(aStartTimeTicks >= 0.0);
		if (!mScene->HasAnimations()) {
			throw avk::runtime_error("Model has no animations");
		}
		if (aAnimationIndex > mScene->mNumAnimations) {
			throw avk::runtime_error("Requested animation index out of bounds");
		}

		double ticksPerSec = mScene->mAnimations[aAnimationIndex]->mTicksPerSecond;
		double durationTicks = mScene->mAnimations[aAnimationIndex]->mDuration;
		double endTicks = glm::min(aEndTimeTicks, durationTicks);

		return animation_clip_data{ aAnimationIndex, ticksPerSec, aStartTimeTicks, endTicks };
	}

	animation model_t::prepare_animation_for_meshes_into_strided_contiguous_memory(uint32_t aAnimationIndex,
	                                                                                 std::vector<mesh_index_t>
	                                                                                 aMeshIndices,
	                                                                                 glm::mat4*
	                                                                                 aBeginningOfTargetStorage,
	                                                                                 size_t aStride,
	                                                                                 std::optional<size_t>
	                                                                                 aMaxNumBoneMatrices)
	{
		if (!aMaxNumBoneMatrices.has_value()) {
			aMaxNumBoneMatrices = aStride;
		}

		animation result;
		for (size_t i = 0; i < aMeshIndices.size(); ++i) {
			result.mMeshIndicesAndTargetStorage.emplace_back(aMeshIndices[i], aBeginningOfTargetStorage + aStride * i);
		}
		result.mMaxNumBoneMatrices = aMaxNumBoneMatrices.value();
		result.mAnimationIndex = aAnimationIndex;

		// --------------------------- helper collections ------------------------------------
		// Contains mappings of bone/node names to aiNode* pointers:
		std::unordered_map<std::string, aiNode*> mapNameToNode;
		add_all_to_node_map(mapNameToNode, mScene->mRootNode);

		// Which node is modified by bone animation? => Only those with an entry in this map:
		std::unordered_map<aiNode*, aiNodeAnim*> mapNodeToBoneAnimation;

		// Prepare mesh-specific things:
		//  - Store the inverse bind pose matrices for each requested mesh's node/bone
		//  - Store the target indices where the bone matrices shall be written to
		//  - Store the root transform matrix (mesh space)
		struct boneMatrixInfo
		{
			bone_mesh_data mBoneMeshData;
			unsigned int mBoneIndex;
		};
		// Matrix information per bone per mesh:
		std::vector<std::unordered_map<aiNode*, boneMatrixInfo>> mapsBoneToMatrixInfo;

		// Which bones have been added per mesh. This is used to keep track of the bones added
		// and also serves to add the then un-added bones in their natural order.
		std::vector<std::vector<bool>> flagsBonesAddedAsAniNodes;

		// At which index has which node been inserted (relevant mostly for keeping track of parent-nodes):
		std::map<aiNode*, size_t> mapNodeToAniNodeIndex;
		// -----------------------------------------------------------------------------------

		// -------------------------------- helper lambdas -----------------------------------
		// Checks whether the given node is modified by bones a.k.a. bone-animated (by searching it in mapNodeToBoneAnimation)
		auto isNodeModifiedByBones = [&](aiNode* bNode) -> bool{
			const auto it = mapNodeToBoneAnimation.find(bNode);
			if (std::end(mapNodeToBoneAnimation) != it) {
				assert(it->first == bNode);
				return true;
			}
			return false;
		};

		// Helper lambda for checking whether a node has already been added and if so, returning its index
		auto isNodeAlreadyAdded = [&](aiNode* bNode) -> std::optional<size_t>{
			auto it = mapNodeToAniNodeIndex.find(bNode);
			if (std::end(mapNodeToAniNodeIndex) != it) {
				return it->second;
			}
			return {};
		};

		// Helper lambda for getting the 'next' parent node which is animated.
		// 'next' means: Next up the parent hierarchy WHICH IS BONE-ANIMATED.
		// If no such parent exists, an empty value will be returned.
		auto getAnimatedParentIndex = [&](aiNode* bNode) -> std::optional<size_t>{
			auto* parent = bNode->mParent;
			while (nullptr != parent) {
				auto already = isNodeAlreadyAdded(parent);
				if (already.has_value()) {
					assert(isNodeModifiedByBones(parent));
					return already;
				}
				else {
					assert(!isNodeModifiedByBones(parent));
				}
				parent = parent->mParent;
			}
			return {};
		};

		// Helper lambda for getting the accumulated parent transforms up the parent
		// hierarchy until a parent node is encountered which is bone-animated. That
		// bone-animated parent is NOT included in the accumulated transformation matrix.
		auto getUnanimatedParentTransform = [&](aiNode* bNode) -> glm::mat4{
			aiMatrix4x4 parentTransform{};
			auto* parent = bNode->mParent;
			while (nullptr != parent) {
				if (!isNodeModifiedByBones(parent)) {
					parentTransform = parent->mTransformation * parentTransform;
					parent = parent->mParent;
				}
				else {
					assert(isNodeAlreadyAdded(parent).has_value());
					parent = nullptr; // stop if the parent is animated
				}
			}
			return to_mat4(parentTransform);
		};

		// Helper-lambda to create an animated_node instance:
		auto addAnimatedNode = [&](aiNodeAnim* bChannel, aiNode* bNode, std::optional<size_t> bAnimatedParentIndex,
		                           const glm::mat4& bUnanimatedParentTransform){
			auto& anode = result.mAnimationData.emplace_back();
			mapNodeToAniNodeIndex[bNode] = result.mAnimationData.size() - 1;

			if (nullptr != bChannel) {
				for (unsigned int i = 0; i < bChannel->mNumPositionKeys; ++i) {
					anode.mPositionKeys.emplace_back(position_key{
						bChannel->mPositionKeys[i].mTime, to_vec3(bChannel->mPositionKeys[i].mValue)
					});
				}
				for (unsigned int i = 0; i < bChannel->mNumRotationKeys; ++i) {
					anode.mRotationKeys.emplace_back(rotation_key{
						bChannel->mRotationKeys[i].mTime, to_quat(bChannel->mRotationKeys[i].mValue)
					});
				}
				for (unsigned int i = 0; i < bChannel->mNumScalingKeys; ++i) {
					anode.mScalingKeys.emplace_back(scaling_key{
						bChannel->mScalingKeys[i].mTime, to_vec3(bChannel->mScalingKeys[i].mValue)
					});
				}
			}

			// Tidy-up the keys:
			//
			// There is one special case which will occur (probably often) in practice. That is, that there
			// are no keys at all (position + rotation + scaling == 0), because the animation does not modify a
			// given bone.
			// Such a case is created in the last step in this function (prepare_animation_for_meshes_into_strided_consecutive_storage),
			// which is looking for bones which have not been animated by Assimp's channels, but need to receive
			// a proper bone matrix.
			//
			// If it is not the special case, then assure that there ARE keys in each of the keys-collections,
			// that will (hopefully) make animation more performant because it requires fewer ifs.
			if (anode.mPositionKeys.size() + anode.mRotationKeys.size() + anode.mScalingKeys.size() > 0) {
				if (anode.mPositionKeys.empty()) {
					// The time doesn't really matter, but do not apply any translation
					anode.mPositionKeys.emplace_back(position_key{0.0, glm::vec3{0.f}});
				}
				if (anode.mRotationKeys.empty()) {
					// The time doesn't really matter, but do not apply any rotation
					anode.mRotationKeys.emplace_back(rotation_key{0.0, glm::quat(1.f, 0.f, 0.f, 0.f)});
				}
				if (anode.mScalingKeys.empty()) {
					// The time doesn't really matter, but do not apply any scaling
					anode.mScalingKeys.emplace_back(scaling_key{0.0, glm::vec3{1.f}});
				}
			}

			// Some lil' optimization flags:
			anode.mSameRotationAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mRotationKeys);
			anode.mSameScalingAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mScalingKeys);

			anode.mAnimatedParentIndex = bAnimatedParentIndex;
			anode.mParentTransform = bUnanimatedParentTransform;
			if (anode.mAnimatedParentIndex.has_value()) {
				assert(!(
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mTransform[0][0] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mTransform[1][1] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mTransform[2][2] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mTransform[3][3] == 0.0f
				));
				anode.mTransform = result.mAnimationData[anode.mAnimatedParentIndex.value()].mTransform * anode.
					mParentTransform;
			}
			else {
				anode.mTransform = anode.mParentTransform;
			}

			// See if we have an inverse bind pose matrix for this node:
			assert(nullptr == bChannel || mapNameToNode.find(to_string(bChannel->mNodeName))->second == bNode);
			for (int i = 0; i < mapsBoneToMatrixInfo.size(); ++i) {
				auto it = mapsBoneToMatrixInfo[i].find(bNode);
				if (std::end(mapsBoneToMatrixInfo[i]) != it) {
					anode.mBoneMeshTargets.emplace_back(it->second.mBoneMeshData);
					flagsBonesAddedAsAniNodes[i][it->second.mBoneIndex] = true;
				}
			}
		};
		// -----------------------------------------------------------------------------------

		// Evaluate the data from the animation and fill mapNodeToBoneAnimation.
		assert(aAnimationIndex >= 0u && aAnimationIndex <= mScene->mNumAnimations);
		auto* ani = mScene->mAnimations[aAnimationIndex];
		for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
			auto* channel = ani->mChannels[i];

			auto it = mapNameToNode.find(to_string(channel->mNodeName));
			if (it == std::end(mapNameToNode)) {
				LOG_ERROR(
					fmt::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.",
						to_string(channel->mNodeName), i));
				continue;
			}

			//if (channel->mNumPositionKeys + channel->mNumRotationKeys + channel->mNumScalingKeys > 0) {
			//requiredForAnimation.insert(it->second);
			mapNodeToBoneAnimation[it->second] = channel;
			//// Also mark all its parent nodes as required for animation (but not modified by bones!):
			//auto* parent = it->second->mParent;
			//while (nullptr != parent) {
			//	requiredForAnimation.insert(parent);
			//	parent = parent->mParent;
			//}
			//}
		}

		//// Checks whether the given node is required for this animation (by searching it in requiredForAnimation)
		//auto isNodeRequiredForAnimation = [&](aiNode* bNode) {
		//	const auto it = requiredForAnimation.find(bNode);
		//	if (std::end(requiredForAnimation) != it) {
		//		assert( *it == bNode );
		//		return true;
		//	}
		//	return false;
		//};

		for (size_t i = 0; i < aMeshIndices.size(); ++i) {
			auto& bmi = mapsBoneToMatrixInfo.emplace_back();
			auto mi = aMeshIndices[i];

			glm::mat4 inverseMeshRootMatrix = glm::inverse(transformation_matrix_for_mesh(mi));

			assert(mi >= 0u && mi < mScene->mNumMeshes);
			flagsBonesAddedAsAniNodes.emplace_back(static_cast<size_t>(mScene->mMeshes[mi]->mNumBones), false);
			// Vector with a flag for each bone

			// For each bone, create boneMatrixInfo:
			for (unsigned int bi = 0; bi < mScene->mMeshes[mi]->mNumBones; ++bi) {
				auto* bone = mScene->mMeshes[mi]->mBones[bi];

				auto it = mapNameToNode.find(to_string(bone->mName));
				if (it == std::end(mapNameToNode)) {
					LOG_ERROR(
						fmt::format("Bone named '{}' could not be found in the nodeMap.", to_string(bone->mName)));
					continue;
				}

				assert(!bmi.contains(it->second));
				bmi[it->second] = boneMatrixInfo{
					bone_mesh_data{
						to_mat4(bone->mOffsetMatrix),
						aBeginningOfTargetStorage + i * aStride + bi,
						inverseMeshRootMatrix
					},
					bi
				};
			}
		}

		// ---------------------------------------------
		// AND NOW: Construct the animated_nodes "tree"
#if _DEBUG
		{
			std::vector<aiNode*> sanityCheck;
			for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
				auto* channel = ani->mChannels[i];
				auto it = mapNameToNode.find(to_string(channel->mNodeName));
				if (it == std::end(mapNameToNode)) {
					sanityCheck.push_back(it->second);
				}
			}
			std::sort(std::begin(sanityCheck), std::end(sanityCheck));
			auto uniqueEnd = std::unique(std::begin(sanityCheck), std::end(sanityCheck));
			if (uniqueEnd != std::end(sanityCheck)) {
				LOG_WARNING(
					fmt::format(
						"Some nodes are contained multiple times in the animation channels of animation[{}]. Don't know if that's going to lead to correct results."
						, aAnimationIndex));
			}
		}
#endif

		// Let's go:
		for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
			auto* channel = ani->mChannels[i];

			auto it = mapNameToNode.find(to_string(channel->mNodeName));
			if (it == std::end(mapNameToNode)) {
				LOG_ERROR(
					fmt::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.",
						to_string(channel->mNodeName), i));
				continue;
			}

			auto* node = it->second;
			std::stack<aiNode*> boneAnimatedParents;
			auto* parent = node->mParent;
			while (nullptr != parent) {
				if (isNodeModifiedByBones(parent) && !isNodeAlreadyAdded(parent).has_value()) {
					boneAnimatedParents.push(parent);
					LOG_DEBUG(
						fmt::format(
							"Interesting: Node '{}' in parent-hierarchy of node '{}' is also bone-animated, but not encountered them while iterating through channels yet."
							, parent->mName.C_Str(), node->mName.C_Str()));
				}
				parent = parent->mParent;
			}

			// First, add the stack of parents, then add the node itself
			while (!boneAnimatedParents.empty()) {
				auto parentToBeAdded = boneAnimatedParents.top();
				assert(mapNodeToBoneAnimation.contains(parentToBeAdded));
				addAnimatedNode(mapNodeToBoneAnimation[parentToBeAdded], parentToBeAdded,
				                getAnimatedParentIndex(parentToBeAdded), getUnanimatedParentTransform(parentToBeAdded));
				boneAnimatedParents.pop();
			}
			addAnimatedNode(mapNodeToBoneAnimation[node], node, getAnimatedParentIndex(node),
			                getUnanimatedParentTransform(node));
		}

		// It could be that there are still bones for which we have not set up an animated_node entry and hence,
		// no bone matrix will be written for them.
		// This happened for all bones which are not affected by the given animation. We must write a bone matrix
		// for them as well => Find them and add them as animated_node entry (but without any position/rotation/scaling keys).
		assert(flagsBonesAddedAsAniNodes.size() == aMeshIndices.size());
		for (size_t i = 0; i < aMeshIndices.size(); ++i) {
			const auto mi = aMeshIndices[i];

			// Set the bone matrices that are not affected by animation ONCE/NOW:
			for (unsigned int bi = 0; bi < mScene->mMeshes[mi]->mNumBones; ++bi) {
				if (flagsBonesAddedAsAniNodes[i][bi]) {
					continue;
				}

				auto* bone = mScene->mMeshes[mi]->mBones[bi];
				auto it = mapNameToNode.find(to_string(bone->mName));
				assert(std::end(mapNameToNode) != it);

				addAnimatedNode(
					nullptr,
					// <-- This is fine. This node is just not affected by animation but still needs to receive bone matrix updates
					it->second, getAnimatedParentIndex(it->second), getUnanimatedParentTransform(it->second)
				);
			}
		}

		return result;
	}

	std::optional<glm::mat4> model_t::transformation_matrix_traverser_for_camera(const aiCamera* aCamera, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		if (aNode->mName == aCamera->mName) {
			return glm::transpose(glm::make_mat4(&nodeM.a1));
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser_for_camera(aCamera, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	void model_t::add_all_to_node_map(std::unordered_map<std::string, aiNode*>& aNodeMap, aiNode* aNode)
	{
		aNodeMap[to_string(aNode->mName)] = aNode;
		for (unsigned int i = 0; i < aNode->mNumChildren; ++i) {
			add_all_to_node_map(aNodeMap, aNode->mChildren[i]);
		}
	}
}
