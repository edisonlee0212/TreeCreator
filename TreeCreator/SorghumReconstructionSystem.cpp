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

void SorghumReconstruction::SorghumReconstructionSystem::DrawGUI()
{
	
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CreatePlant(std::string path, float resolution, std::string name) const
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
				auto front = glm::normalize(spline->EvaluateAxis(percent));
				auto up = glm::vec3(0.0f, 0.0f, 1.0f);
				
				up = glm::normalize(glm::cross(glm::cross(front, up), front));
				float width;
				if(percent < 0.2f)
				{
					width = percent;
				}else if(percent > 0.8f)
				{
					width = 1.0f - percent * 0.9f;
				}else
				{
					width = 0.2f;
				}
				spline->Segments.emplace_back(spline->EvaluatePoint(percent), up, front, width, 180.0f * (1.0f - percent * 0.6f));
			}
			//Truck
			const int step = 10;

			const int vertexIndex = spline->Vertices.size();
			Vertex archetype;
			const float xStep = 1.0f / step * 2.0f;
			const float yStep = 1.0f / spline->Segments.size();
			for (int i = 0; i < spline->Segments.size(); i++)
			{
				auto& segment = spline->Segments.at(i);
				const float angleStep = segment.Theta / step;
				const int vertsCount = step * 2 + 1;
				for (int j = 0; j < vertsCount; j++)
				{
					archetype.Position = segment.GetPoint((j - step) * angleStep);
					archetype.TexCoords0 = glm::vec2(j * xStep, j * yStep);
					spline->Vertices.push_back(archetype);
				}
				if (i != 0) {
					for (int j = 0; j < vertsCount - 1; j++) {
						//Down triangle
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j);
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j + 1);
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j);
						//Up triangle
						spline->Indices.push_back(vertexIndex + (i - 1) * vertsCount + j + 1);
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j);
						spline->Indices.push_back(vertexIndex + ((i - 1) + 1) * vertsCount + j + 1);
					}
				}
			}


		}
	);
	//auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(truck);
	//truckSpline = EntityManager::GetSharedComponent<Spline>(truck);

	//mmc->Mesh = std::make_shared<Mesh>();
	//mmc->Mesh->SetVertices(17, truckSpline->Vertices, truckSpline->Indices);

	EntityManager::ForEachChild(truck, [](Entity child)
		{
			auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(child);
			auto childSpline = EntityManager::GetSharedComponent<Spline>(child);
			mmc->Mesh = std::make_shared<Mesh>();
			mmc->Mesh->SetVertices(17, childSpline->Vertices, childSpline->Indices, true);
		}
	);
	ExportPlant(truck, name);
	return truck;
}

void SorghumReconstruction::SorghumReconstructionSystem::ExportPlant(Entity plant, std::string path) const
{
	std::ofstream of;
	of.open((path + ".obj").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string start = "#Plant model, by Bosheng Li";
		start += "\n";
		of.write(start.c_str(), start.size());
		of.flush();

		unsigned startIndex = 1;
		EntityManager::ForEachChild(plant, [&of, &startIndex](Entity child)
			{
				auto lt = EntityManager::GetComponentData<LocalTranslation>(child).Value;
				auto mesh = EntityManager::GetSharedComponent<MeshMaterialComponent>(child)->Mesh;
				if (!mesh->GetVerticesUnsafe()->empty() && !mesh->GetIndicesUnsafe()->empty())
				{
					std::string header = "#Vertices: " + std::to_string(mesh->GetVerticesUnsafe()->size()) + ", tris: " + std::to_string(mesh->GetIndicesUnsafe()->size() / 3);
					header += "\n";
					of.write(header.c_str(), header.size());
					of.flush();
					std::string o = "o ";
					o +=
						"["
						+ std::to_string(lt.x) + ","
						+ std::to_string(lt.z)
						+ "]" + "\n";
					of.write(o.c_str(), o.size());
					of.flush();
					std::string data;
#pragma region Data collection

					for (const auto& vertex : *mesh->GetVerticesUnsafe()) {
						data += "v " + std::to_string(vertex.Position.x + lt.x)
							+ " " + std::to_string(vertex.Position.z + lt.z)
							+ " " + std::to_string(vertex.Position.y + lt.y)
							+ "\n";
					}

					//data += "s off\n";
					data += "# List of indices for faces vertices, with (x, y, z).\n";
					for (auto i = 0; i < mesh->GetIndicesUnsafe()->size() / 3; i++) {
						auto f1 = mesh->GetIndicesUnsafe()->at(3l * i) + startIndex;
						auto f2 = mesh->GetIndicesUnsafe()->at(3l * i + 1) + startIndex;
						auto f3 = mesh->GetIndicesUnsafe()->at(3l * i + 2) + startIndex;
						data += "f " + std::to_string(f1)
							+ " " + std::to_string(f2)
							+ " " + std::to_string(f3)
							+ "\n";
					}
					startIndex += mesh->GetVerticesUnsafe()->size();
#pragma endregion
					of.write(data.c_str(), data.size());
					of.flush();

				}
			}
		);


		of.close();
		Debug::Log("Plant saved as " + path + ".obj");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void SorghumReconstruction::SorghumReconstructionSystem::OnCreate()
{
	_TruckArchetype = EntityManager::CreateEntityArchetype("Truck",
		TruckInfo(),
		Translation(), Rotation(), Scale(), LocalToWorld()
	);
	_LeafArchetype = EntityManager::CreateEntityArchetype("Leaf",
		LeafInfo(),
		LocalTranslation(), LocalRotation(), LocalScale(), LocalToParent(), LocalToWorld()
	);
	_SplineQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAnyFilters(_SplineQuery, TruckInfo(), LeafInfo());

	_TruckMaterial = std::make_shared<Material>();
	_TruckMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseTruck->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	_TruckMaterial->Textures2Ds()->push_back(textureDiffuseTruck);

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureLeaf = new Texture2D(TextureType::DIFFUSE);
	textureLeaf->LoadTexture(FileIO::GetResourcePath("Textures/green.png"), "");
	_LeafMaterial->Textures2Ds()->push_back(textureLeaf);
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
