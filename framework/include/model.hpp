#pragma once
#include <gvk.hpp>

namespace gvk
{
	using model_index_t = size_t;
	using mesh_index_t = size_t;

	struct animation_clip_data
	{
		unsigned int mAnimationIndex;
		double mTicksPerSecond;
		double mStartTicks;
		double mEndTicks;
	};

	struct position_key
	{
		double mTime;
		glm::vec3 mValue;
	};

	struct rotation_key
	{
		double mTime;
		glm::quat mValue;
	};

	struct scaling_key
	{
		double mTime;
		glm::vec3 mValue;
	};

	struct animated_node
	{
		/**	Animation keys for the positions of this node. */
		std::vector<position_key> mPositionKeys;

		/**	Animation keys for the rotations of this node. */
		std::vector<rotation_key> mRotationKeys;

		/**	Animation keys for the scalings of this node. */
		std::vector<scaling_key> mScalingKeys;

		bool mSameRotationAndPositionKeyTimes;
		bool mSameScalingAndPositionKeyTimes;
		
		/** The GLOBAL transform of this node */
		glm::mat4 mTransform;

		///**	Holds either the index of the parent node if the parent node is
		// *	also affected by animation; or holds the parent's global transform
		// *	matrix if the parent is not affected by animation.
		// */
		//std::variant<size_t, glm::mat4> mParentIndexOrTransform;

		/** Contains the index of a parent node IF this node HAS a parent
		 *	node that is affected by animation.
		 */
		std::optional<size_t> mAnimatedParentIndex;

		/** Parent transform that must be applied to this node.
		 *
		 *	IF this node has an mAnimatedParentIndex, the mParentTransform
		 *	must be applied BEFORE the animated parent's transform is applied!
		 */
		glm::mat4 mParentTransform;

		/**	Contains values only if this node IS a bone. If set, it contains
		 *	one or multiple matrices that transform from bone space to mesh
		 *	space in bind pose.
		 *	This is called "inverse bind pose matrix" or "offset matrix".
		 *
		 *	There will be multiple matrices contained if the animation has
		 *	been created for multiple meshes. It can contain at most one
		 *	entry if the animation is created for one mesh only.
		 *	The mesh index it refers to is not stored within this struct.
		 *	(It could probably be calculated from the offsets stored in
		 *	 mBoneMatrixTargets, though.)
		 *
		 *	Only if mInversePoseMatrix has a value, the mBoneMatrixTargets
		 *	collection might (should?!) contain one or multiple values.
		 *
		 *	This vector is perfectly aligned with mBoneMatrixTargets, i.e.
		 *	same size, can and should be processed in lockstep.
		 */
		std::vector<glm::mat4> mInverseBindPoseMatrix;

		/**	This vector contains target pointers into target storage where
		 *	resulting bone matrices are to be written to.
		 *	This will only contain values if mInverseBindPoseMatrix is set.
		 *	(I.e. only if this node represents a bone.)
		 *
		 *	This vector is perfectly aligned with mInverseBindPoseMatrix, i.e.
		 *	same size, can and should be processed in lockstep.
		 */
		std::vector<glm::mat4*> mBoneMatrixTargets;

		/**	For each mesh, this vector contains the mesh root node's inverse 
		 *	matrix; i.e. the matrix that changes the basis of given coordinates
		 *	into mesh space.
		 *
		 *	Similarly to the matrices in mInverseBindPoseMatrix, these matrices
		 *	transform into mesh space. Only these matrices transform from object
		 *	space into mesh space while mInverseBindPoseMatrix transform from
		 *	bone space to mesh space.
		 *
		 *	This vector is perfectly aligned with mInverseBindPoseMatrix, i.e.
		 *	same size, can and should be processed in lockstep.
		 */
		std::vector<glm::mat4> mInverseMeshRootMatrix;
	};

	struct animation
	{
		/** Collection of tuples with two elements:
		 *  [0]: The mesh index to be animated
		 *  [1]: Pointer to the target storage where bone matrices shall be written to
		 */
		std::vector<std::tuple<mesh_index_t, glm::mat4*>> mMeshIndicesAndTargetStorage;

		/** Maximum number of bone matrices to write into the target storage pointers.
		 *  (Second tuple element of mMeshIndicesAndTargetStorage)
		 */
		size_t mMaxNumBoneMatrices;

		/**	All animated nodes, along with their animation data and target storage pointers
		 */
		std::vector<animated_node> mAnimationData;
	};

	class model_t
	{
		friend class context_vulkan;

	public:
		using aiProcessFlagsType = unsigned int;

		model_t() = default;
		model_t(model_t&&) noexcept = default;
		model_t(const model_t&) = delete;
		model_t& operator=(model_t&&) noexcept = default;
		model_t& operator=(const model_t&) = delete;
		~model_t() = default;

		const auto* handle() const { return mScene; }

		static avk::owning_resource<model_t> load_from_file(const std::string& aPath, aiProcessFlagsType aAssimpFlags = aiProcess_Triangulate);

		static avk::owning_resource<model_t> load_from_memory(const std::string& aMemory, aiProcessFlagsType aAssimpFlags = aiProcess_Triangulate);

		/** Returns this model's path where it has been loaded from */
		auto path() const { return mModelPath; }

		/** Determine the transformation matrix for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Transformation matrix of the given mesh, can be the identity
		 */
		glm::mat4 transformation_matrix_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets the name of the mesh at the given index (not to be confused with the material's name)
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Mesh name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_mesh(mesh_index_t _MeshIndex) const;

		/** Gets Assimp's internal material index for the given mesh index.
		 *	This value won't be useful if not operating directly on Assimp's internal materials.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Mesh index corresponding to Assimp's internal materials structure.
		 */
		size_t material_index_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets the name of material at the given material index
		 *	@param		aMaterialIndex		The index corresponding to the material
		 *	@return		Material name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_material(size_t aMaterialIndex) const;

		/** Gets the `material_config` struct for the mesh at the given index.
		 *	The `material_config` struct is created from Assimp's internal material data.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		`material_config` struct, representing the "type of material".
		 *				To actually load all the resources it refers to, you'll have
		 *				to create a `material` based on it.
		 */
		material_config material_config_for_mesh(mesh_index_t aMeshIndex);

		/**	Sets some material config struct for the mesh at the given index.
		 *	@param	aMeshIndex			The index corresponding to the mesh
		 *	@param	aMaterialConfig		Material config that is to be assigned to the mesh.
		 */
		void set_material_config_for_mesh(mesh_index_t aMeshIndex, const material_config& aMaterialConfig);

		/**	Gets all distinct `material_config` structs foor this model and, as a bonus, so to say,
		 *	also gets all the mesh indices which have the materials assigned to.
		 *	@param	aAlsoConsiderCpuOnlyDataForDistinctMaterials	Setting this parameter to `true` means that for determining if a material is unique or not,
		 *															also the data in the material struct are evaluated which only remain on the CPU. This CPU
		 *															data will not be transmitted to the GPU. By default, this parameter is set to `false`, i.e.
		 *															only the GPU data of the `material_config` struct will be evaluated when determining the distinct
		 *															materials.
		 *															You'll want to set this parameter to `true` if you are planning to adapt your draw calls based
		 *															on one or all of the following `material_config` members: `mShadingModel`, `mWireframeMode`,
		 *															`mTwosided`, `mBlendMode`. If you don't plan to differentiate based on these, set to `false`.
		 *	@return	A `std::unordered_map` containing the distinct `material_config` structs as the
		 *			keys and a vector of mesh indices as the value type, i.e. `std::vector<size_t>`.
		 */
		std::unordered_map<material_config, std::vector<mesh_index_t>> distinct_material_configs(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials = false);

		/** Gets the number of vertices for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Number of vertices, which is also the length of all the vectors,
		 *				which are returned by: `positions_for_mesh`, `normals_for_mesh`,
		 *				`tangents_for_mesh`, `bitangents_for_mesh`, `colors_for_mesh`,
		 *				and `texture_coordinates_for_mesh`
		 */
		inline size_t number_of_vertices_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the positions for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of vertex positions, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> positions_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the normals for the mesh at the given index.
		 *	If the mesh has no normals, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,0,1) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of normals, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> normals_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the tangents for the mesh at the given index.
		 *	If the mesh has no tangents, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of tangents, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> tangents_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the bitangents for the mesh at the given index.
		 *	If the mesh has no bitangents, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,1,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of bitangents, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> bitangents_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the colors of a specific color set for the mesh at the given index.
		 *	If the mesh has no colors for the given set index, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,1,1) in this case (magenta).
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of colors
		 *	@return		Vector of colors, converted to `glm::vec4`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec4> colors_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const;

		/** Gets all the bone weights for the mesh at the given index.
		 *	If the mesh has no bone weights, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,0,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of bone weights, converted to `glm::vec4`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec4> bone_weights_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the bone indices for the mesh at the given index.
		 *	If the mesh has no bone indices, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,0,0,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of bone indices, converted to `glm::uvec4`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::uvec4> bone_indices_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets the number of uv-components of a specific UV-set for the mesh at the given index
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of texture coordinates
		 *	@return		Number of uv components the given set has. This can, e.g., be used to
		 *				determine how to retrieve the texture coordinates: as vec2 or as vec3,
		 *				like follows: `texture_coordinates_for_mesh<vec2>(0)` or `texture_coordinates_for_mesh<vec3>(0)`, respectively.
		 */
		int num_uv_components_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const;

		/** Gets all the texture coordinates of a UV-set for the mesh at the given index.
		 *	If the mesh has no colors for the given set index, a vector filled with values is
		 *	returned regardless. You'll have to specify the type of UV-coordinates which you
		 *	want to retrieve. Supported types are `glm::vec2` and `glm::vec3`.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of UV-coordinates
		 *	@return		Vector of UV-coordinates, converted to `T`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		template <typename T> std::vector<T> texture_coordinates_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const
		{
			throw gvk::logic_error(fmt::format("unsupported type {}", typeid(T).name()));
		}

		/** Gets the number of indices for the mesh at the given index.
		 *	Please note: Theoretically it can happen that a mesh has faces with different
		 *	numbers of vertices (e.g. triangles and quads). Use the `aiProcess_Triangulate`
		 *	import flag to get only triangles, or make sure to handle them properly.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Number of indices for the given mesh.
		 */
		int number_of_indices_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the indices for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of vertex positions, converted to type `T`
		 *				of length `number_of_indices_for_mesh()`.
		 *				In most cases, you'll want to pass `uint16_t` or `uint32_t` for `T`.
		 */
		template <typename T>
		std::vector<T> indices_for_mesh(mesh_index_t aMeshIndex) const
		{
			const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
			size_t indicesCount = number_of_indices_for_mesh(aMeshIndex);
			std::vector<T> result;
			result.reserve(indicesCount);
			for (unsigned int i = 0; i < paiMesh->mNumFaces; ++i) {
				// we're working with triangulated meshes only
				const aiFace& paiFace = paiMesh->mFaces[i];
				for (unsigned int f = 0; f < paiFace.mNumIndices; ++f) {
					result.emplace_back(static_cast<T>(paiFace.mIndices[f]));
				}
			}
			return result;
		}

		/** Returns the number of meshes. */
		mesh_index_t num_meshes() const { return mScene->mNumMeshes; }

		/** Return the indices of all meshes which the given predicate evaluates true for.
		 *	Function-signature: bool(size_t, const aiMesh*) where the first parameter is the
		 *									mesh index and the second the pointer to the data
		 */
		template <typename F>
		std::vector<size_t> select_meshes(F aPredicate) const
		{
			std::vector<size_t> result;
			for (size_t i = 0; i < mScene->mNumMeshes; ++i) {
				const aiMesh* paiMesh = mScene->mMeshes[i];
				if (aPredicate(i, paiMesh)) {
					result.push_back(i);
				}
			}
			return result;
		}

		/** Return the indices of all meshes. It's effecively the same as calling
		 *	`select_meshes` with a predicate that always evaluates true.
		 */
		std::vector<size_t> select_all_meshes() const;

		std::vector<glm::vec3> positions_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> normals_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> tangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> bitangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec4> colors_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet = 0) const;
		std::vector<glm::vec4> bone_weights_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::uvec4> bone_indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;

		template <typename T>
		std::vector<T> texture_coordinates_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet = 0) const
		{
			std::vector<T> result;
			for (auto meshIndex : aMeshIndices) {
				auto tmp = texture_coordinates_for_mesh<T>(meshIndex, aSet);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		template <typename T>
		std::vector<T> indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
		{
			std::vector<T> result;
			for (auto meshIndex : aMeshIndices) {
				auto tmp = indices_for_mesh<T>(meshIndex);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		/** Returns all lightsources stored in the model file */
		std::vector<lightsource> lights() const;

		/** Returns all cameras stored in the model file */
		std::vector<gvk::camera> cameras() const;

		/** Load an animation clip's data */
		animation_clip_data load_animation_clip(unsigned int aAnimationIndex, double aStartTimeTicks, double aEndTimeTicks) const;



		static glm::mat4 aiMat4_to_glmMat4(const aiMatrix4x4 &ai)
		{
			glm::mat4 g;
			g[0][0] = ai[0][0]; g[0][1] = ai[1][0]; g[0][2] = ai[2][0]; g[0][3] = ai[3][0];
			g[1][0] = ai[0][1]; g[1][1] = ai[1][1]; g[1][2] = ai[2][1]; g[1][3] = ai[3][1];
			g[2][0] = ai[0][2]; g[2][1] = ai[1][2]; g[2][2] = ai[2][2]; g[2][3] = ai[3][2];
			g[3][0] = ai[0][3]; g[3][1] = ai[1][3]; g[3][2] = ai[2][3]; g[3][3] = ai[3][3];
			return g;
		}

		static glm::vec3 aiVec3_to_glmVec3(const aiVector3D &ai)
		{
			return glm::vec3(ai.x, ai.y, ai.z);
		}

		template<typename T>
		static double getInterpolationFrames(T *keys, size_t numKeys, double ticks, double duration, size_t &priorFrame, size_t &nextFrame)
		{
			// find next key frame (after current time)
			assert(numKeys > 0);
			for (size_t i = 1; i < numKeys; i++) {
				if (ticks <= keys[i].mTime) {
					priorFrame = i - 1;
					nextFrame = i;
					return (ticks - keys[priorFrame].mTime) / (keys[nextFrame].mTime - keys[priorFrame].mTime);
				}
			}
			// interpolate between last and first frame ... does that ever happen?
			priorFrame = numKeys - 1;
			nextFrame = 0;
			return (ticks - keys[priorFrame].mTime) / (keys[nextFrame].mTime + duration - keys[priorFrame].mTime);
		}

		// Returns a 4x4 matrix with interpolated translation between current and next frame
		aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D translation;

			if (pNodeAnim->mNumPositionKeys == 1) {
				translation = pNodeAnim->mPositionKeys[0].mValue;
			}
			else {
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
					if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				translation = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Translation(translation, mat);
			return mat;
		}

		// Returns a 4x4 matrix with interpolated rotation between current and next frame
		aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiQuaternion rotation;

			if (pNodeAnim->mNumRotationKeys == 1) {
				rotation = pNodeAnim->mRotationKeys[0].mValue;
			}
			else {
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
					if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
						frameIndex = i;
						break;
					}
				}

				aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
				aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiQuaternion& start = currentFrame.mValue;
				const aiQuaternion& end = nextFrame.mValue;

				aiQuaternion::Interpolate(rotation, start, end, delta);
				rotation.Normalize();
			}

			aiMatrix4x4 mat(rotation.GetMatrix());
			return mat;
		}


		// Returns a 4x4 matrix with interpolated scaling between current and next frame
		aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D scale;

			if (pNodeAnim->mNumScalingKeys == 1) {
				scale = pNodeAnim->mScalingKeys[0].mValue;
			}
			else {
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
					if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				scale = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Scaling(scale, mat);
			return mat;
		}


		// Find animation for a given node
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName)
		{
			for (uint32_t i = 0; i < animation->mNumChannels; i++) {
				const aiNodeAnim* nodeAnim = animation->mChannels[i];
				if (std::string(nodeAnim->mNodeName.data) == nodeName) {
					return nodeAnim;
				}
			}
			return nullptr;
		}

		// Load bone information from ASSIMP mesh
		std::optional<uint32_t> getBoneIndex(const aiMesh* pMesh, std::string aName)
		{
			for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
				if (std::string(pMesh->mBones[i]->mName.data) == aName) {
					return i;
				}
			}
			return {};
		}

		std::tuple<bool, aiMatrix4x4> findRootMeshNodeTransform(aiNode* meshRoot, mesh_index_t aMeshIndex)
		{
			auto result = std::make_tuple(true, meshRoot->mTransformation);
			for (int i = 0; i < meshRoot->mNumMeshes; ++i) {
				if (meshRoot->mMeshes[i] == aMeshIndex) {
					return result;
				}
			}
			for (int i = 0; i < meshRoot->mNumChildren; ++i) {
				auto result = findRootMeshNodeTransform(meshRoot->mChildren[i], aMeshIndex);
				if (std::get<bool>(result)) {
					return std::make_tuple(true, meshRoot->mTransformation * std::get<aiMatrix4x4>(result));
				}
			}
			return std::make_tuple(false, meshRoot->mTransformation); // wrong node, but better than nothing, I guess
		}

		// Get node hierarchy for current animation time
		void readNodeHierarchy(glm::mat4* aBoneMatricesStorage, mesh_index_t aMeshIndex, const animation_clip_data& aAnimationClip, float aAnimationTime, aiNode* node, const aiMatrix4x4& parentTransform, animated_meshes& aMeshesToAnimate)
		{
			std::string NodeName(node->mName.data);

			aiMatrix4x4 NodeTransformation(node->mTransformation);

			const aiNodeAnim* pNodeAnim = findNodeAnim(mScene->mAnimations[aAnimationClip.mAnimationIndex], NodeName);

			if (pNodeAnim) {
				// Get interpolated matrices between current and next frame
				aiMatrix4x4 matScale = interpolateScale(aAnimationTime, pNodeAnim);
				aiMatrix4x4 matRotation = interpolateRotation(aAnimationTime, pNodeAnim);
				aiMatrix4x4 matTranslation = interpolateTranslation(aAnimationTime, pNodeAnim);

				NodeTransformation = matTranslation * matRotation * matScale;
			}

			aiMatrix4x4 GlobalTransformation = parentTransform * NodeTransformation;

			if (aMeshesToAnimate.boneMapping[aMeshIndex].find(NodeName) != aMeshesToAnimate.boneMapping[aMeshIndex].end()) {
				uint32_t boneIndex = aMeshesToAnimate.boneMapping[aMeshIndex][NodeName];

				// Find the root node OF THE MESH, because "inverse pose matrices" a.k.a. "offset matrices" are relative to a mesh's root
				auto globalInverseTransform = std::get<aiMatrix4x4>(findRootMeshNodeTransform(mScene->mRootNode, aMeshIndex));
				globalInverseTransform.Inverse();

				auto finalTransform = globalInverseTransform * GlobalTransformation * aMeshesToAnimate.boneOffsets[aMeshIndex][boneIndex];

				auto& target = aBoneMatricesStorage[boneIndex];
				target[0][0] = finalTransform.a1;
				target[0][1] = finalTransform.b1;
				target[0][2] = finalTransform.c1;
				target[0][3] = finalTransform.d1;
				target[1][0] = finalTransform.a2;
				target[1][1] = finalTransform.b2;
				target[1][2] = finalTransform.c2;
				target[1][3] = finalTransform.d2;
				target[2][0] = finalTransform.a3;
				target[2][1] = finalTransform.b3;
				target[2][2] = finalTransform.c3;
				target[2][3] = finalTransform.d3;
				target[3][0] = finalTransform.a4;
				target[3][1] = finalTransform.b4;
				target[3][2] = finalTransform.c4;
				target[3][3] = finalTransform.d4;
			}

			for (uint32_t i = 0; i < node->mNumChildren; i++) {
				readNodeHierarchy(aBoneMatricesStorage, aMeshIndex, aAnimationClip, aAnimationTime, node->mChildren[i], GlobalTransformation, aMeshesToAnimate);
			}
		}

		void initNodeAnimMap(std::map<aiNode *, aiNodeAnim *>& m_nodeToNodeAnimMap, aiAnimation *m_animation, aiNode * node)
		{
			if (m_nodeToNodeAnimMap.find(node) == m_nodeToNodeAnimMap.end()) {
				for (size_t i = 0; i < m_animation->mNumChannels; i++) {
					if (m_animation->mChannels[i]->mNodeName == node->mName) {
						m_nodeToNodeAnimMap[node] = m_animation->mChannels[i];
						break;
					}
				}
			}
			for (size_t i = 0; i < node->mNumChildren; i++)
				initNodeAnimMap(m_nodeToNodeAnimMap, m_animation, node->mChildren[i]);
		}

		animated_meshes declare_to_animate_all_meshes_into_strided_consecutive_storage(glm::mat4* aBeginningOfTargetStorage, size_t aStride, std::optional<size_t> aMaxNumBoneMatrices = {})
		{
			if (!aMaxNumBoneMatrices.has_value()) {
				aMaxNumBoneMatrices = aStride;
			}

			auto numMeshes = num_meshes();
			animated_meshes result;
			for (decltype(numMeshes) i = 0; i < numMeshes; ++i) {
				result.mMeshIndicesAndTargetStorage.emplace_back(i, aBeginningOfTargetStorage + i * aStride);
			}
			result.mMaxNumBoneMatrices = aMaxNumBoneMatrices.value();

			for (decltype(numMeshes) i = 0; i < numMeshes; ++i) {
				auto pMesh = mScene->mMeshes[i];

				auto& bm = result.boneMapping.emplace_back();
				auto& om = result.boneOffsets.emplace_back();
				om.resize(pMesh->mNumBones);

				auto numBones = 0u;

				for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
					uint32_t index = 0;

					assert(pMesh->mNumBones <= aMaxNumBoneMatrices.value());

					std::string name(pMesh->mBones[i]->mName.data);

					if (bm.find(name) == bm.end()) {
						// Bone not present, add new one
						index = numBones;
						numBones++;
						om[index] = pMesh->mBones[i]->mOffsetMatrix;
						bm[name] = index;
					}
					else {
						index = bm[name];
					}
				}
			}


			return result;
		}

		animated_meshes declare_to_animate_all_meshes_into_tightly_packed_consecutive_storage(glm::mat4* aBeginningOfTargetStorage, size_t aMaxNumBoneMatrices)
		{
			return declare_to_animate_all_meshes_into_strided_consecutive_storage(aBeginningOfTargetStorage, aMaxNumBoneMatrices, aMaxNumBoneMatrices);
		}

		void update_bone_matrices(animated_meshes& aMeshesToAnimate, const animation_clip_data& aAnimationClip, double aTime)
		{
			for (auto& aniMesh : aMeshesToAnimate.mMeshIndicesAndTargetStorage) {
				aiMatrix4x4 identity = aiMatrix4x4();
				auto meshIndex = std::get<mesh_index_t>(aniMesh);
				auto* targetStorage = std::get<glm::mat4*>(aniMesh);
				readNodeHierarchy(targetStorage, meshIndex, aAnimationClip, aTime, mScene->mRootNode, identity, aMeshesToAnimate);
			}
		}

		static glm::vec3 to_vec3(const aiVector3D& aAssimpVector)
		{
			return *reinterpret_cast<const glm::vec3*>(&aAssimpVector.x);
		}

		static glm::quat to_quat(const aiQuaternion& aAssimpQuat)
		{
			return glm::quat(aAssimpQuat.w, aAssimpQuat.x, aAssimpQuat.y, aAssimpQuat.z);
		}

		static glm::mat4 to_mat4(const aiMatrix4x4& aAssimpMat)
		{
			return glm::transpose(*reinterpret_cast<const glm::mat4*>(&aAssimpMat[0][0]));
		}

		static std::string to_string(const aiString& aAssimpString)
		{
			return std::string(aAssimpString.C_Str());
		}

		void add_to_node_map(std::unordered_map<std::string, aiNode*>& aNodeMap, aiNode* aNode)
		{
			aNodeMap[to_string(aNode->mName)] = aNode;
			for (unsigned int i = 0; i < aNode->mNumChildren; ++i) {
				add_to_node_map(aNodeMap, aNode->mChildren[i]);
			}
		}

		template <typename T1, typename T2>
		bool have_same_key_times(const T1& aCollection1, const T2& aCollection2)
		{
			if (aCollection1.size() != aCollection2.size()) {
				return false;
			}
			for (size_t i = 0; i < aCollection1.size(); ++i) {
				if (glm::abs(aCollection1[i].mTime - aCollection2[i].mTime) > std::numeric_limits<double>::epsilon()) {
					return false;
				}
			}
			return true;
		}

		animation prepare_animation_for_meshes_into_strided_consecutive_storage(uint32_t aAnimationIndex, std::vector<mesh_index_t> aMeshIndices, glm::mat4* aBeginningOfTargetStorage, size_t aStride, std::optional<size_t> aMaxNumBoneMatrices = {})
		{
			if (!aMaxNumBoneMatrices.has_value()) {
				aMaxNumBoneMatrices = aStride;
			}

			animation result;
			for (size_t i = 0; i < aMeshIndices.size(); ++i) {
				result.mMeshIndicesAndTargetStorage.emplace_back(aMeshIndices[i], aBeginningOfTargetStorage + aStride * i);
			}
			result.mMaxNumBoneMatrices = aMaxNumBoneMatrices.value();

			// Contains mappings of bone/node names to aiNode* pointers:
			std::unordered_map<std::string, aiNode*> nodeMap;
			add_to_node_map(nodeMap, mScene->mRootNode);

			// Gather some information about the nodes:
			//  - Is the node even required?
			//  - Is the node being animated through bone animation or does it remain static within the given animation?
			//std::unordered_set<aiNode*> requiredForAnimation;
			std::unordered_map<aiNode*, aiNodeAnim*> modifiedByBones;
			// Evaluate the data from the animation and fill ^those two maps.
			assert(aAnimationIndex >= 0u && aAnimationIndex <= mScene->mNumAnimations);
			auto* ani = mScene->mAnimations[aAnimationIndex];
			for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
				auto* channel = ani->mChannels[i];

				auto it = nodeMap.find(to_string(channel->mNodeName));
				if (it == std::end(nodeMap)) {
					LOG_ERROR(fmt::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.", to_string(channel->mNodeName), i));
					continue;
				}

				if (channel->mNumPositionKeys + channel->mNumRotationKeys + channel->mNumScalingKeys > 0) {
					//requiredForAnimation.insert(it->second);
					modifiedByBones[it->second] = channel;
					//// Also mark all its parent nodes as required for animation (but not modified by bones!):
					//auto* parent = it->second->mParent;
					//while (nullptr != parent) {
					//	requiredForAnimation.insert(parent);
					//	parent = parent->mParent;
					//}
				}
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

			// Checks whether the given node is modified by bones a.k.a. bone-animated (by searching it in modifiedByBones)
			auto isNodeModifiedByBones = [&](aiNode* bNode) -> bool {
				const auto it = modifiedByBones.find(bNode);
				if (std::end(modifiedByBones) != it) {
					assert( it->first == bNode );
					return true;
				}
				return false;
			};

			// Prepare mesh-specific things:
			//  - Store the inverse bind pose matrices for each requested mesh's node/bone
			//  - Store the target indices where the bone matrices shall be written to
			//  - Store the root transform matrix (mesh space)
			struct boneMatrixInfo
			{
				glm::mat4 mInverseBindPoseMatrix;
				glm::mat4* mTargetStoragePointer;
				glm::mat4 mMeshRootMatrix;
			};
			std::unordered_map<aiNode*, boneMatrixInfo> boneToMatrixInfo; // TODO: Is this REALLY unique across all meshes or should it be std::vector<std::unordered_map<...>> i.e. a node can affect multiple meshes
			for (size_t i = 0; i < aMeshIndices.size(); ++i) {
				auto mi = aMeshIndices[i];

				glm::mat4 meshRootMatrix = transformation_matrix_for_mesh(mi)
				
				assert(mi >= 0u && mi < mScene->mNumMeshes);
				for (unsigned int bi = 0; bi < mScene->mMeshes[mi]->mNumBones; ++bi) {
					auto* bone = mScene->mMeshes[mi]->mBones[bi];

					auto it = nodeMap.find(to_string(bone->mName));
					if (it == std::end(nodeMap)) {
						LOG_ERROR(fmt::format("Bone named '{}' could not be found in the nodeMap.", to_string(bone->mName)));
						continue;
					}

					assert (!boneToMatrixInfo.contains(it->second)); // Must be uniquely associable to one mesh, otherwise the logic is flawed <===== TODO: true?
					boneToMatrixInfo[it->second] = boneMatrixInfo {
						to_mat4(bone->mOffsetMatrix),
						aBeginningOfTargetStorage + i * aStride + bi,
						meshRootMatrix
					};
				}
			}

			// ---------------------------------------------
			// AND NOW: Construct the animated_nodes "tree"

			// At which index has which node been inserted (relevant mostly for keeping track of parent-nodes):
			std::map<aiNode*, size_t> animatedNodeToIndex;

			// Helper lambda for checking whether a node has already been added and if so, returning its index
			auto isNodeAlreadyAdded = [&](aiNode* bNode) -> std::optional<size_t> {
				auto it = animatedNodeToIndex.find(bNode);
				if (std::end(animatedNodeToIndex) != it) {
					return it->second;
				}
				return {};
			};

			// Helper lambda for getting the 'next' parent node which is animated.
			// 'next' means: Next up the parent hierarchy WHICH IS BONE-ANIMATED.
			// If no such parent exists, an empty value will be returned.
			auto getAnimatedParentIndex = [&](aiNode* bNode) -> std::optional<size_t> {
				auto* parent = bNode->mParent;
				while (nullptr != parent) {
					auto already = isNodeAlreadyAdded(parent);
					if (already.has_value()) {
						assert (isNodeModifiedByBones(parent));
						return already;
					}
					else {
						assert (!isNodeModifiedByBones(parent));
					}
					parent = parent->mParent;
				}
				return {};
			};

			// Helper lambda for getting the accumulated parent transforms up the parent
			// hierarchy until a parent node is encountered which is bone-animated. That
			// bone-animated parent is NOT included in the accumulated transformation matrix.
			auto getUnanimatedParentTransform = [&](aiNode* bNode) -> glm::mat4 {
				aiMatrix4x4 parentTransform{};
				auto* parent = bNode->mParent;
				while (nullptr != parent) {
					if (!isNodeModifiedByBones(parent)) {
						assert (isNodeAlreadyAdded(parent).has_value());
						parentTransform = parent->mTransformation * parentTransform;
						parent = parent->mParent;
					}
					else {
						parent = nullptr; // stop if the parent is animated
					}
				}
				return to_mat4(parentTransform);
			};

			// Helper-lambda to create an animated_node instance:
			auto addAnimatedNode = [&](aiNodeAnim* bChannel, aiNode* bNode, std::optional<size_t> bAnimatedParentIndex, const glm::mat4& bUnanimatedParentTransform)
			{
				auto& anode = result.mAnimationData.emplace_back();
				animatedNodeToIndex[bNode] = result.mAnimationData.size() - 1;
				
				for (unsigned int i = 0; i < bChannel->mNumPositionKeys; ++i) {
					anode.mPositionKeys.emplace_back(position_key{ bChannel->mPositionKeys[i].mTime, to_vec3(bChannel->mPositionKeys[i].mValue) });
				}
				for (unsigned int i = 0; i < bChannel->mNumRotationKeys; ++i) {
					anode.mRotationKeys.emplace_back(rotation_key{ bChannel->mRotationKeys[i].mTime, to_quat(bChannel->mRotationKeys[i].mValue) });
				}
				for (unsigned int i = 0; i < bChannel->mNumScalingKeys; ++i) {
					anode.mScalingKeys.emplace_back(scaling_key{ bChannel->mScalingKeys[i].mTime, to_vec3(bChannel->mScalingKeys[i].mValue) });
				}

				// Tidy-up the keys:
				if (anode.mPositionKeys.size() == 0) {
					anode.mPositionKeys.emplace_back(position_key{ std::numeric_limits<double>::min(), glm::vec3{0.f} });
					anode.mPositionKeys.emplace_back(position_key{ std::numeric_limits<double>::max(), glm::vec3{0.f} });
				}
				if (anode.mPositionKeys.size() == 1) {
					anode.mPositionKeys.emplace_back(anode.mPositionKeys.front()); // copy
					anode.mPositionKeys.front().mTime = std::numeric_limits<double>::min();
					anode.mPositionKeys.back().mTime = std::numeric_limits<double>::max();
				}
				if (anode.mRotationKeys.size() == 0) {
					anode.mRotationKeys.emplace_back(rotation_key{ std::numeric_limits<double>::min(), glm::quat(1.f, 0.f, 0.f, 0.f) });
					anode.mRotationKeys.emplace_back(rotation_key{ std::numeric_limits<double>::max(), glm::quat(1.f, 0.f, 0.f, 0.f) });
				}
				if (anode.mRotationKeys.size() == 1) {
					anode.mRotationKeys.emplace_back(anode.mRotationKeys.front()); // copy
					anode.mRotationKeys.front().mTime = std::numeric_limits<double>::min();
					anode.mRotationKeys.back().mTime = std::numeric_limits<double>::max();
				}
				if (anode.mScalingKeys.size() == 0) {
					anode.mScalingKeys.emplace_back(scaling_key{ std::numeric_limits<double>::min(), glm::vec3{1.f} });
					anode.mScalingKeys.emplace_back(scaling_key{ std::numeric_limits<double>::max(), glm::vec3{1.f} });
				}
				if (anode.mScalingKeys.size() == 1) {
					anode.mScalingKeys.emplace_back(anode.mScalingKeys.front()); // copy
					anode.mScalingKeys.front().mTime = std::numeric_limits<double>::min();
					anode.mScalingKeys.back().mTime = std::numeric_limits<double>::max();
				}

				// Some lil' optimization flags:
				anode.mSameRotationAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mRotationKeys);
				anode.mSameScalingAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mScalingKeys);
				
				anode.mAnimatedParentIndex = bAnimatedParentIndex;
				anode.mParentTransform = bUnanimatedParentTransform;

				// See if we have an inverse bind pose matrix for this node:
				assert(nodeMap.find(to_string(bChannel->mNodeName))->second == bNode);
				auto it = boneToMatrixInfo.find(bNode);
				// TODO: If std::unordered_map<aiNode*, boneMatrixInfo> boneToMatrixInfo; is turned into std::vector<std::unordered_map<aiNode*, boneMatrixInfo>>, there must be a loop:
				if (std::end(boneToMatrixInfo) != it) {
					anode.mInverseBindPoseMatrix.emplace_back(it->second.mInverseBindPoseMatrix);
					anode.mBoneMatrixTargets.emplace_back(it->second.mTargetStoragePointer);
					anode.mInverseMeshRootMatrix.emplace_back(glm::inverse(it->second.mMeshRootMatrix));
				}
			};

#if _DEBUG
			{
				std::vector<aiNode*> sanityCheck;
				for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
					auto* channel = ani->mChannels[i];
					auto it = nodeMap.find(to_string(channel->mNodeName));
					if (it == std::end(nodeMap)) {
						sanityCheck.push_back(it->second);
					}
				}
				std::sort(std::begin(sanityCheck), std::end(sanityCheck));
				auto uniqueEnd = std::unique(std::begin(sanityCheck), std::end(sanityCheck));
				if (uniqueEnd != std::end(sanityCheck)) {
					LOG_WARNING(fmt::format("Some nodes are contained multiple times in the animation channels of animation[{}]. Don't know if that's going to lead to correct results.", aAnimationIndex));
				}
			}
#endif

			// Let's go:
			for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
				auto* channel = ani->mChannels[i];

				auto it = nodeMap.find(to_string(channel->mNodeName));
				if (it == std::end(nodeMap)) {
					LOG_ERROR(fmt::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.", to_string(channel->mNodeName), i));
					continue;
				}

				auto* node = it->second;
				std::stack<aiNode*> boneAnimatedParents;
				auto* parent = node->mParent;
				while (nullptr != parent) {
					if (isNodeModifiedByBones(parent) && !isNodeAlreadyAdded(parent).has_value()) {
						boneAnimatedParents.push(parent);
						LOG_DEBUG(fmt::format("Interesting: Node '{}' in parent-hierarchy of node '{}' is also bone-animated, but not encountered them while iterating through channels yet.", parent->mName.C_Str(), node->mName.C_Str()));
					}
					parent = parent->mParent;
				}

				// First, add the stack of parents, then add the node itself
				while (!boneAnimatedParents.empty()) {
					auto parentToBeAdded = boneAnimatedParents.top();
					assert( modifiedByBones.contains(parentToBeAdded) );
					addAnimatedNode(modifiedByBones[parentToBeAdded], parentToBeAdded, getAnimatedParentIndex(parentToBeAdded), getUnanimatedParentTransform(parentToBeAdded));
					boneAnimatedParents.pop();
				}
				addAnimatedNode(modifiedByBones[node], node, getAnimatedParentIndex(node), getUnanimatedParentTransform(node));
			}

			return result;
		}

		template <typename T>
		std::tuple<size_t, size_t> find_positions_in_keys(const T& aCollection, double aTime)
		{
			size_t pos1 = 0, pos2 = 0;
			while (aCollection[pos1].mTime > aTime && pos1 < aCollection.size() - 1) {
				++pos1;
			}
			if (aCollection.size() > 1) {
				pos2 = pos1 + 1;
				while (aCollection[pos2].mTime < aTime && pos2 < aCollection.size() - 1) {
					LOG_WARNING(fmt::format("Now that's strange: keys[{}].mTime {} > {}, despite keys[{}].mTime {} <= {}", pos2, aCollection[pos2].mTime, aTime, pos1, aCollection[pos1].mTime, aTime));
					++pos2;
				}
			}
			return std::make_tuple(pos1, pos2);
		}

		template <typename K>
		float get_interpolation_factor(const K& key1, const K& key2, double aTime)
		{
			assert (key2.mTime >= key1.mTime);
			double timeDifferenceTicks = key2.mTime - key1.mTime;
			return static_cast<float>((aTime - key1) / timeDifferenceTicks);
		}
		
		auto animate(animation& aAnimation, const animation_clip_data& aClip, double mTime)
		{
			if (aClip.mTicksPerSecond == 0.0) {
				throw gvk::runtime_error("mTicksPerSecond may not be 0.0 => set a different value!");
			}

			double timeInTicks = mTime * aClip.mTicksPerSecond;

			for (auto& aniNode : aAnimation.mAnimationData) {
				// Translation/position:
				auto [tpos1, tpos2] = find_positions_in_keys(aniNode.mPositionKeys, timeInTicks);
				auto tf = get_interpolation_factor(aniNode.mPositionKeys[tpos1], aniNode.mPositionKeys[tpos2], timeInTicks);
				auto translation = glm::lerp(aniNode.mPositionKeys[tpos1].mValue, aniNode.mPositionKeys[tpos2].mValue, tf);
				
				// Rotation:
				size_t rpos1 = tpos1, rpos2 = tpos2;
				if (!aniNode.mSameRotationAndPositionKeyTimes) {
					std::tie(rpos1, rpos2) = find_positions_in_keys(aniNode.mRotationKeys, timeInTicks);
				}
				auto rf = get_interpolation_factor(aniNode.mRotationKeys[rpos1], aniNode.mRotationKeys[rpos2], timeInTicks);
				auto rotation = glm::lerp(aniNode.mRotationKeys[rpos1].mValue, aniNode.mRotationKeys[rpos2].mValue, rf);

				// Scaling:
				size_t spos1 = tpos1, spos2 = tpos2;
				if (!aniNode.mSameScalingAndPositionKeyTimes) {
					std::tie(spos1, spos2) = find_positions_in_keys(aniNode.mScalingKeys, timeInTicks);
				}
				auto sf = get_interpolation_factor(aniNode.mScalingKeys[spos1], aniNode.mScalingKeys[spos2], timeInTicks);
				auto scaling = glm::lerp(aniNode.mScalingKeys[spos1].mValue, aniNode.mScalingKeys[spos2].mValue, sf);

				auto localTransform = matrix_from_transforms(translation, rotation, scaling);
				if (aniNode.mAnimatedParentIndex.has_value()) {
					aniNode.mTransform = aAnimation.mAnimationData[aniNode.mAnimatedParentIndex.value()].mTransform * aniNode.mParentTransform * localTransform;
				}
				else {
					aniNode.mTransform = aniNode.mParentTransform * localTransform;
				}

				const auto n = aniNode.mInverseBindPoseMatrix.size();
				assert (n == aniNode.mBoneMatrixTargets.size());
				for (size_t i = 0; i < n; ++i) {
					glm::mat4 boneMatrix = glm::inverse()
				}
			}
		}

	private:
		void initialize_materials();
		std::optional<glm::mat4> transformation_matrix_traverser(const unsigned int aMeshIndexToFind, const aiNode* aNode, const aiMatrix4x4& aM) const;
		std::optional<glm::mat4> transformation_matrix_traverser_for_light(const aiLight* aLight, const aiNode* Node, const aiMatrix4x4& aM) const;
		std::optional<glm::mat4> transformation_matrix_traverser_for_camera(const aiCamera* aCamera, const aiNode* aNode, const aiMatrix4x4& aM) const;

		std::unique_ptr<Assimp::Importer> mImporter;
		std::string mModelPath;
		const aiScene* mScene;
		std::vector<std::optional<material_config>> mMaterialConfigPerMesh;
	};

	using model = avk::owning_resource<model_t>;


	template <>
	inline std::vector<glm::vec2> model_t::texture_coordinates_for_mesh<glm::vec2>(mesh_index_t _MeshIndex, int _Set) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec2> result;
		result.reserve(n);
		assert(_Set >= 0 && _Set < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[_Set]) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0) for each vertex.", _MeshIndex, _Set));
			result.emplace_back(0.f, 0.f);
		}
		else {
			const auto nuv = num_uv_components_for_mesh(_MeshIndex, _Set);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[_Set][i][0], 0.f);
				}
				break;
			case 2:
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[_Set][i][0], paiMesh->mTextureCoords[_Set][i][1]);
				}
				break;
			default:
				throw gvk::logic_error(fmt::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, _MeshIndex, _Set));
			}
		}
		return result;
	}

	template <>
	inline std::vector<glm::vec3> model_t::texture_coordinates_for_mesh<glm::vec3>(mesh_index_t _MeshIndex, int _Set) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		assert(_Set >= 0 && _Set < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[_Set]) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0,0) for each vertex.", _MeshIndex, _Set));
			result.emplace_back(0.f, 0.f, 0.f);
		}
		else {
			const auto nuv = num_uv_components_for_mesh(_MeshIndex, _Set);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[_Set][i][0], 0.f, 0.f);
				}
				break;
			case 2:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[_Set][i][0], paiMesh->mTextureCoords[_Set][i][1], 0.f);
				}
				break;
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[_Set][i][0], paiMesh->mTextureCoords[_Set][i][1], paiMesh->mTextureCoords[_Set][i][2]);
				}
				break;
			default:
				throw gvk::logic_error(fmt::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, _MeshIndex, _Set));
			}
		}
		return result;
	}

	/** Helper function used by `cgb::append_indices_and_vertex_data` */
	template <typename Vert>
	size_t get_vertex_count(const Vert& _First)
	{
		return _First.size();
	}

	/** Helper function used by `cgb::append_indices_and_vertex_data` */
	template <typename Vert, typename... Verts>
	size_t get_vertex_count(const Vert& _First, const Verts&... _Rest)
	{
#if defined(_DEBUG) 
		// Check whether all of the vertex data has the same length!
		auto countOfNext = get_vertex_count(_Rest...);
		if (countOfNext != _First.size()) {
			throw gvk::logic_error(fmt::format("The vertex data passed are not all of the same length, namely {} vs. {}.", countOfNext, _First.size()));
		}
#endif
		return _First.size();
	}

	/** Inserts the elements from the collection `_ToInsert` into the collection `_Destination`. */
	template <typename V>
	void insert_into(V& _Destination, const V& _ToInsert)
	{
		_Destination.insert(std::end(_Destination), std::begin(_ToInsert), std::end(_ToInsert));
	}

	/** Inserts the elements from the collection `_ToInsert` into the collection `_Destination` and adds `_ToAdd` to them. */
	template <typename V, typename A>
	void insert_into_and_add(V& _Destination, const V& _ToInsert, A _ToAdd)
	{
		_Destination.reserve(_Destination.size() + _ToInsert.size());
		auto addValType = static_cast<typename V::value_type>(_ToAdd);
		for (auto& e : _ToInsert) {
			_Destination.push_back(e + addValType);
		}
	}

	/** Utility function to concatenate lists of vertex data and according lists of index data.
	 *	The vertex data is concatenated unmodified, and an arbitrary number of vertex data vectors is supported.
	 *	The index data, however, will be modified during concatenation to account for the vertices which come before.
	 *
	 *	Example:
	 *	If there are already 100 vertices in the vertex data vectors, adding the indices 0, 2, 1 will result in
	 *	actually the values 100+0, 100+2, 100+1, i.e. 100, 102, 101, being added to the vector of existing indices.
	 *
	 *	Usage:
	 *	This method takes `std::tuple`s as parameters to assign source collections to destination collections.
	 *	The destinations are referring to collections, while the sources must be lambdas providing the data.
	 *		Example: `std::vector<glm::vec3> positions;` for the first parameter and `[&]() { return someModel->positions_for_meshes({ 0 }); }` for the second parameter.
	 *	Please note that the first parameter of these tuples is captured by reference, which requires
	 *	`std::forward_as_tuple` to be used. For better readability, `cgb::additional_index_data` and
	 *	`cgb::additional_vertex_data` can be used instead, which are actually just the same as `std::forward_as_tuple`.
	 *
	 *
	 */
	template <typename... Vert, typename... Getter, typename Ind, typename IndGetter>
	void append_indices_and_vertex_data(std::tuple<Ind&, IndGetter> _IndDstAndGetter, std::tuple<Vert&, Getter>... _VertDstAndGetterPairs)
	{
		// Count vertices BEFORE appending!
		auto vertexCount = get_vertex_count(std::get<0>(_VertDstAndGetterPairs)...);
		// Append all the vertex data:
		(insert_into(/* Existing vector: */ std::get<0>(_VertDstAndGetterPairs), /* Getter: */ std::move(std::get<1>(_VertDstAndGetterPairs)())), ...);
		// Append the index data:
		insert_into_and_add(std::get<0>(_IndDstAndGetter), std::get<1>(_IndDstAndGetter)(), vertexCount);
		//insert_into_and_add(_A, _B(), vertexCount);
	}

	/** This is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	 *	but it should help to express the intent better. Use it with `cgb::append_vertex_data_and_indices`!
	 */
	template <class... _Types>
	_NODISCARD constexpr std::tuple<_Types&&...> additional_vertex_data(_Types&&... _Args) noexcept
	{
		return std::forward_as_tuple(std::forward<_Types>(_Args)...);
	}

	/** This is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	 *	but it should help to express the intent better. Use it with `cgb::append_vertex_data_and_indices`!
	 */
	template <class... _Types>
	_NODISCARD constexpr std::tuple<_Types&&...> additional_index_data(_Types&&... _Args) noexcept
	{
		return std::forward_as_tuple(std::forward<_Types>(_Args)...);
	}

	///** This is a convenience function and is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	// *	but it should help to express the intent better. 
	// */
	//template <typename M>
	//_NODISCARD constexpr std::tuple<std::reference_wrapper<model_t>, std::vector<size_t>> make_tuple_model_and_indices(const M& _Model, std::vector<mesh_index_t> _Indices) noexcept {
	//	return std::forward_as_tuple<std::reference_wrapper<model_t>, std::vector<size_t>>(std::ref(_Model), std::move(_Indices));
	//}


}
