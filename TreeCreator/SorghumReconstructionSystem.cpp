#include "SorghumReconstructionSystem.h"


Entity SorghumReconstruction::SorghumReconstructionSystem::CreatePlant() const
{
	Entity ret = EntityManager::CreateEntity(_PlantArchetype);
	Scale s;
	s.Value = glm::vec3(1.0f);
	auto spline = std::make_shared<Spline>();
	EntityManager::SetSharedComponent(ret, spline);
	EntityManager::SetComponentData(ret, s);
	auto mmc = std::make_shared<MeshMaterialComponent>();
	mmc->Material = _PlantMaterial;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CreateLeafForPlant(Entity& plantEntity) const
{
	Entity ret = EntityManager::CreateEntity(_LeafArchetype);
	EntityManager::SetParent(ret, plantEntity);
	LocalScale ls;
	ls.Value = glm::vec3(1.0f);
	auto spline = std::make_shared<Spline>();
	EntityManager::SetSharedComponent(ret, spline);
	EntityManager::SetComponentData(ret, ls);

	auto mmc = std::make_shared<MeshMaterialComponent>();
	mmc->Material = _LeafMaterial;
	mmc->Mesh = nullptr;
	mmc->BackCulling = false;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

void SorghumReconstruction::SorghumReconstructionSystem::DrawGUI()
{
	
}

void SorghumReconstruction::SorghumReconstructionSystem::GenerateMeshForAllPlants()
{
	EntityManager::ForEach<LocalToWorld>(_SplineQuery, []
	(int index, Entity entity, LocalToWorld* ltw)
		{

			auto spline = EntityManager::GetSharedComponent<Spline>(entity);
			spline->Vertices.clear();
			spline->Indices.clear();
			spline->Segments.clear();
			/*const int amount = 30;
			for (int i = 0; i < amount; i++)
			{
				float percent = (float)i / (amount - 1);
				auto front = glm::normalize(spline->EvaluateAxisFromCurve(percent));

				auto up = glm::normalize(glm::cross(spline->Left, front));
			
				spline->Segments.emplace_back(spline->EvaluatePointFromCurve(percent), up, front, spline->EvaluateWidth(percent), spline->EvaluateTheta(percent));
			}*/

			const int amount = 10;

			for(int i = 1; i < spline->Nodes.size(); i++)
			{
				auto& prev = spline->Nodes.at(i - 1);
				auto& curr = spline->Nodes.at(i);
				
				float distance = glm::distance(prev.Position, curr.Position);
				BezierCurve curve = BezierCurve(prev.Position, prev.Position + distance / 5.0f * prev.Axis, curr.Position - distance / 5.0f * curr.Axis, curr.Position);
				for(float div = 1.0f / amount; div <= 1.0f; div += 1.0f / amount)
				{
					auto front = prev.Axis * (1.0f - div) + curr.Axis * div;
					
					auto up = glm::normalize(glm::cross(spline->Left, front));
					
					spline->Segments.emplace_back(
						curve.GetPoint(div),
						up, 
						front,
						prev.Width * (1.0f - div) + curr.Width * div,
						prev.Theta * (1.0f - div) + curr.Theta * div);
				}
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
	std::vector<Entity> plants;
	_PlantQuery.ToEntityArray(plants);
	for (auto& plant : plants) {
		EntityManager::ForEachChild(plant, [](Entity child)
			{
				auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(child);
				auto childSpline = EntityManager::GetSharedComponent<Spline>(child);
				mmc->Mesh = std::make_shared<Mesh>();
				mmc->Mesh->SetVertices(17, childSpline->Vertices, childSpline->Indices, true);
			}
		);
	}
	
}

Entity SorghumReconstruction::SorghumReconstructionSystem::ImportPlant(std::string path, float resolution, std::string name) const
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
	Entity truck = CreatePlant();
	truck.SetName(name);
	auto truckSpline = EntityManager::GetSharedComponent<Spline>(truck);



	truckSpline->StartingPoint = -1;
	truckSpline->Import(file);

	//Recenter plant:
	glm::vec3 posSum = truckSpline->Curves.front().CP0;
	
	/*
	int pointAmount = 0;
	for (auto& curve : truckSpline->Curves) {
		pointAmount += 2;
		posSum += curve.CP0;
		posSum += curve.CP3;
	}
	posSum /= pointAmount;
	*/
	for (auto& curve : truckSpline->Curves) {
		curve.CP0 -= posSum;
		curve.CP1 -= posSum;
		curve.CP2 -= posSum;
		curve.CP3 -= posSum;
	}
	EntityManager::SetSharedComponent(truck, truckSpline);
	float minOrder = 10.0f;
	for (int i = 0; i < leafCount; i++) {
		Entity leaf = CreateLeafForPlant(truck);
		auto leafSpline = EntityManager::GetSharedComponent<Spline>(leaf);
		float startingPoint;
		file >> startingPoint;
		
		if (minOrder > startingPoint) minOrder = startingPoint;
		leafSpline->StartingPoint = startingPoint;
		leafSpline->Import(file);
		EntityManager::SetSharedComponent(leaf, leafSpline);
		for (auto& curve : leafSpline->Curves) {
			curve.CP0 += truckSpline->EvaluatePointFromCurve(startingPoint);
			curve.CP1 += truckSpline->EvaluatePointFromCurve(startingPoint);
			curve.CP2 += truckSpline->EvaluatePointFromCurve(startingPoint);
			curve.CP3 += truckSpline->EvaluatePointFromCurve(startingPoint);
		}

		leafSpline->Left = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), leafSpline.get()->Curves.begin()->CP0 - leafSpline.get()->Curves.back().CP3);
	}
	EntityManager::ForEachChild(truck, [&minOrder, &truckSpline](Entity leaf)
		{
			auto leafSpline = EntityManager::GetSharedComponent<Spline>(leaf);

			//leafSpline->BuildStem(truckSpline);
			leafSpline->BuildNodes(truckSpline);
		});
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
	_PlantArchetype = EntityManager::CreateEntityArchetype("Truck",
		SorghumInfo(),
		Translation(), Rotation(), Scale(), LocalToWorld()
	);
	_LeafArchetype = EntityManager::CreateEntityArchetype("Leaf",
		LeafInfo(),
		LocalTranslation(), LocalRotation(), LocalScale(), LocalToParent(), LocalToWorld()
	);
	_SplineQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAnyFilters(_SplineQuery, SorghumInfo(), LeafInfo());
	_PlantQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_PlantQuery, SorghumInfo());
	_PlantMaterial = std::make_shared<Material>();
	_PlantMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseTruck->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	_PlantMaterial->Textures2Ds()->push_back(textureDiffuseTruck);

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
	Curves.clear();
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


void SorghumReconstruction::Spline::BuildNodes(std::shared_ptr<Spline>& truckSpline)
{
	float width = 0.1f - StartingPoint * 0.05f;
	for(float i = 0.0f; i < StartingPoint - 0.05f; i += 0.05f)
	{
		Nodes.emplace_back(truckSpline.get()->EvaluatePointFromCurve(i), 180.0f, width, truckSpline.get()->EvaluateAxisFromCurve(i));
	}
	for(float i = 0.05f; i <= 1.0f; i += 0.05f)
	{
		float w = 0.15f;
		if (i > 0.75f) w -= (i - 0.75f) * 0.5f;
		Nodes.emplace_back(EvaluatePointFromCurve(i), i == 0.05f ? 20.0f : 10.0f, w, EvaluateAxisFromCurve(i));
	}
	
}
