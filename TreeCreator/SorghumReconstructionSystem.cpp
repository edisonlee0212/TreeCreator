#include "SorghumReconstructionSystem.h"


Entity SorghumReconstruction::SorghumReconstructionSystem::CreateTruck() const
{
	Entity ret = EntityManager::CreateEntity(_TruckArchetype);
	Scale s;
	s.Value = glm::vec3(1.0f);
	auto spline = std::make_shared<Spline>();
	EntityManager::SetSharedComponent(ret, spline);
	EntityManager::SetComponentData(ret, s);
	auto mmc = std::make_shared<MeshMaterialComponent>();
	mmc->Material = _TruckMaterial;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CreateLeafForTruck(Entity& truckEntity) const
{
	Entity ret = EntityManager::CreateEntity(_LeafArchetype);
	EntityManager::SetParent(ret, truckEntity);
	LocalScale ls;
	ls.Value = glm::vec3(1.0f);
	auto spline = std::make_shared<Spline>();
	EntityManager::SetSharedComponent(ret, spline);
	EntityManager::SetComponentData(ret, ls);

	auto mmc = std::make_shared<MeshMaterialComponent>();
	mmc->Material = _LeafMaterial;
	mmc->Mesh = nullptr;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CreatePlant(std::string path, float resolution) const
{
	std::ifstream file(path, std::fstream::in);
	if (!file.is_open())
	{
		Debug::Log("Failed to open file!");
		return Entity();
	}
	// Number of leaves in the file
	int leafCount;
	file >> leafCount;
	Entity truck = CreateTruck();
	auto truckSpline = EntityManager::GetSharedComponent<Spline>(truck);



	truckSpline->StartingPoint = -1;
	truckSpline->Import(file);

	//Recenter plant:
	glm::vec3 posSum = glm::vec3(0.0f);
	int pointAmount = 0;
	for (auto& curve : truckSpline->Curves) {
		pointAmount += 2;
		posSum += curve.CP0;
		posSum += curve.CP3;
	}
	posSum /= pointAmount;
	for (auto& curve : truckSpline->Curves) {
		curve.CP0 -= posSum;
		curve.CP1 -= posSum;
		curve.CP2 -= posSum;
		curve.CP3 -= posSum;
	}

	EntityManager::SetSharedComponent(truck, truckSpline);
	for (int i = 0; i < leafCount; i++) {
		Entity leaf = CreateLeafForTruck(truck);
		auto leafSpline = EntityManager::GetSharedComponent<Spline>(leaf);
		float startingPoint;
		file >> startingPoint;
		leafSpline->StartingPoint = startingPoint;
		leafSpline->Import(file);
		EntityManager::SetSharedComponent(leaf, leafSpline);



		LocalTranslation lt;
		lt.Value = truckSpline->EvaluatePoint(startingPoint);
		EntityManager::SetComponentData(leaf, lt);
	}
	//TODO: fix this
	EntityManager::ForEach<LocalToWorld>(_SplineQuery, [resolution]
	(int index, Entity entity, LocalToWorld* ltw)
		{

			auto spline = EntityManager::GetSharedComponent<Spline>(entity);
			spline->Vertices.clear();
			spline->Indices.clear();
			spline->Segments.clear();
			const int amount = 15;
			for (int i = 0; i < amount; i++)
			{
				float percent = (float)i / (amount - 1);
				auto front = spline->EvaluateAxis(percent);
				auto up = glm::vec3(1.0f);
				up = glm::cross(glm::cross(front, up), front);
				spline->Segments.emplace_back(spline->EvaluatePoint(percent), glm::quatLookAt(front, up), 0.3f, 180.0f * (1.0f - percent * 0.6f));
			}
			//Truck
			const int step = 10;

			const int vertexIndex = spline->Vertices.size();
			Vertex archetype;
			const float xStep = 1.0f / step * 2.0f;
			const float yStep = 1.0f / spline->Segments.size();
			for (int i = 0; i < spline->Segments.size(); i++)
			{
				const auto& segment = spline->Segments.at(i);
				const float angleStep = segment.Theta / step;
				const int vertsCount = step * 2 + 1;
				for (int j = 0; j < vertsCount; j++)
				{
					archetype.Position = spline->Segments.at(i).GetPoint(j * angleStep);
					archetype.TexCoords0 = glm::vec2(j * xStep, j * yStep);
					spline->Vertices.push_back(archetype);
				}
				if (i != 0) {
					for (int j = 0; j < vertsCount - 1; j++) {
						//Down triangle
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j);
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j + 1);
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j);
						//Up triangle
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j + 1);
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j);
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j + 1);
					}
				}
			}


		}
	);
	auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(truck);
	truckSpline = EntityManager::GetSharedComponent<Spline>(truck);

	mmc->Mesh = std::make_shared<Mesh>();
	mmc->Mesh->SetVertices(17, truckSpline->Vertices, truckSpline->Indices);

	EntityManager::ForEachChild(truck, [](Entity child)
		{
			auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(child);
			auto childSpline = EntityManager::GetSharedComponent<Spline>(child);
			mmc->Mesh = std::make_shared<Mesh>();
			mmc->Mesh->SetVertices(17, childSpline->Vertices, childSpline->Indices);
		}
	);
	return truck;
}

void SorghumReconstruction::SorghumReconstructionSystem::OnCreate()
{
	_TruckArchetype = EntityManager::CreateEntityArchetype("Truck",
		TruckInfo(),
		Translation(), Rotation(), Scale(), LocalToWorld(), MeshMaterialComponent()
	);
	_LeafArchetype = EntityManager::CreateEntityArchetype("Leaf",
		LeafInfo(),
		LocalTranslation(), LocalRotation(), LocalScale(), LocalToParent(), LocalToWorld(), MeshMaterialComponent()
	);
	_SplineQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAnyFilters(_SplineQuery, TruckInfo(), LeafInfo());

	_TruckMaterial = std::make_shared<Material>();
	_TruckMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseTruck->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	auto textureNormalTruck = new Texture2D(TextureType::NORMAL);
	textureNormalTruck->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_normal.jpg"), "");
	_TruckMaterial->Textures2Ds()->push_back(textureDiffuseTruck);
	_TruckMaterial->Textures2Ds()->push_back(textureNormalTruck);

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureLeaf = new Texture2D(TextureType::DIFFUSE);
	textureLeaf->LoadTexture(FileIO::GetResourcePath("Textures/green.png"), "");
	auto textureNormalLeaf = new Texture2D(TextureType::NORMAL);
	textureNormalLeaf->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"), "");
	_LeafMaterial->Textures2Ds()->push_back(textureLeaf);
	_LeafMaterial->Textures2Ds()->push_back(textureNormalLeaf);
	Enable();
}

void SorghumReconstruction::SorghumReconstructionSystem::OnDestroy()
{
}

void SorghumReconstruction::SorghumReconstructionSystem::Update()
{

}

void SorghumReconstruction::SorghumReconstructionSystem::FixedUpdate()
{
}

void SorghumReconstruction::Spline::Import(std::ifstream& stream)
{
	int curveAmount;
	stream >> curveAmount;
	for (int i = 0; i < curveAmount; i++) {
		glm::vec3 cp[4];
		float x, y, z;
		for (auto& j : cp)
		{
			stream >> x >> y >> z;
			j = glm::vec3(x, y, z) * 10.0f;
		}
		Curves.emplace_back(cp[0], cp[1], cp[2], cp[3]);
	}
}
