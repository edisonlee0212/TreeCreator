#include "SorghumReconstructionSystem.h"


Entity SorghumReconstruction::SorghumReconstructionSystem::CreatePlant() const
{
	Entity ret = EntityManager::CreateEntity(_PlantArchetype);
	Scale s;
	s.Value = glm::vec3(1.0f);
	auto spline = std::make_shared<Spline>();
	EntityManager::SetSharedComponent(ret, spline);
	EntityManager::SetComponentData(ret, s);
	
	auto mmc = std::make_shared<MeshRenderer>();
	mmc->Material = _StemMaterial;
	mmc->Mesh = std::make_shared<Mesh>();
	mmc->BackCulling = false;
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

	auto mmc = std::make_shared<MeshRenderer>();
	mmc->Material = _LeafMaterial;
	mmc->Mesh = std::make_shared<Mesh>();
	mmc->BackCulling = false;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

void SorghumReconstruction::SorghumReconstructionSystem::DrawGUI()
{
	
}

void SorghumReconstruction::SorghumReconstructionSystem::ObjExportHelper(glm::vec3 position, std::shared_ptr<Mesh> mesh, std::ofstream& of,
	unsigned& startIndex) const
{
	if (!mesh->GetVerticesUnsafe()->empty() && !mesh->GetIndicesUnsafe()->empty())
	{
		std::string header = "#Vertices: " + std::to_string(mesh->GetVerticesUnsafe()->size()) + ", tris: " + std::to_string(mesh->GetIndicesUnsafe()->size() / 3);
		header += "\n";
		of.write(header.c_str(), header.size());
		of.flush();
		std::string o = "o ";
		o +=
			"["
			+ std::to_string(position.x) + ","
			+ std::to_string(position.z)
			+ "]" + "\n";
		of.write(o.c_str(), o.size());
		of.flush();
		std::string data;
#pragma region Data collection

		for (const auto& vertex : *mesh->GetVerticesUnsafe()) {
			data += "v " + std::to_string(vertex.Position.x + position.x)
				+ " " + std::to_string(vertex.Position.z + position.z)
				+ " " + std::to_string(vertex.Position.y + position.y)
				+ "\n";
		}
		for (const auto& vertex : *mesh->GetVerticesUnsafe()) {
			data += "vn " + std::to_string(vertex.Normal.x)
				+ " " + std::to_string(vertex.Normal.z)
				+ " " + std::to_string(vertex.Normal.y)
				+ "\n";
		}
		
		for (const auto& vertex : *mesh->GetVerticesUnsafe()) {
			data += "vt " + std::to_string(vertex.TexCoords0.x)
				+ " " + std::to_string(vertex.TexCoords0.y)
				+ "\n";
		}
		//data += "s off\n";
		data += "# List of indices for faces vertices, with (x, y, z).\n";
		for (auto i = 0; i < mesh->GetIndicesUnsafe()->size() / 3; i++) {
			auto f1 = mesh->GetIndicesUnsafe()->at(3l * i) + startIndex;
			auto f2 = mesh->GetIndicesUnsafe()->at(3l * i + 1) + startIndex;
			auto f3 = mesh->GetIndicesUnsafe()->at(3l * i + 2) + startIndex;
			data += "f " + std::to_string(f1) + "/" + std::to_string(f1) + "/" + std::to_string(f1)
				+ " " + std::to_string(f2) + "/" + std::to_string(f2) + "/" + std::to_string(f2)
				+ " " + std::to_string(f3) + "/" + std::to_string(f3) + "/" + std::to_string(f3)
				+ "\n";
		}
		startIndex += mesh->GetVerticesUnsafe()->size();
#pragma endregion
		of.write(data.c_str(), data.size());
		of.flush();
	}
}


void SorghumReconstruction::SorghumReconstructionSystem::GenerateMeshForAllPlants()
{
	std::mutex meshMutex;
	EntityManager::ForEach<LocalToWorld>(_SplineQuery, [&meshMutex]
	(int index, Entity entity, LocalToWorld* ltw)
		{

			auto spline = EntityManager::GetSharedComponent<Spline>(entity);
			spline->Nodes.clear();
			int stemNodeCount = 0;
			if (spline->StartingPoint != -1) {
				auto truckSpline = EntityManager::GetSharedComponent<Spline>(EntityManager::GetParent(entity));
				float width = 0.1f - spline->StartingPoint * 0.05f;
				for (float i = 0.0f; i < spline->StartingPoint - 0.05f; i += 0.05f)
				{
					spline->Nodes.emplace_back(truckSpline->EvaluatePointFromCurve(i), 180.0f, width, truckSpline->EvaluateAxisFromCurve(i));

				}
				stemNodeCount = spline->Nodes.size();
				for (float i = 0.05f; i <= 1.0f; i += 0.05f)
				{
					float w = 0.15f;
					if (i > 0.75f) w -= (i - 0.75f) * 0.5f;
					spline->Nodes.emplace_back(spline->EvaluatePointFromCurve(i), i == 0.05f ? 60.0f : 40.0f, w, spline->EvaluateAxisFromCurve(i));
				}
			}else
			{
				for (float i = 0.0f; i <= 1.0f; i += 0.05f)
				{
					spline->Nodes.emplace_back(spline->EvaluatePointFromCurve(i), 180.0f, 0.04f, spline->EvaluateAxisFromCurve(i));
				}
				auto endPoint = spline->EvaluatePointFromCurve(1.0f);
				auto endAxis = spline->EvaluateAxisFromCurve(1.0f);
				spline->Nodes.emplace_back(endPoint + endAxis * 0.05f, 10.0f, 0.001f, endAxis);
				stemNodeCount = spline->Nodes.size();
			}
			spline->Vertices.clear();
			spline->Indices.clear();
			spline->Segments.clear();

			const int amount = 5;
			float temp = 0.0f;

			float leftPeriodFactor = glm::linearRand(0.4f, 1.0f);
			float rightPeriodFactor = glm::linearRand(0.4f, 1.0f);
			float leftFlatness = glm::linearRand(0.75f, 1.75f);
			float rightFlatness = glm::linearRand(0.5f, 1.5f);
			float leftFlatnessFactor = glm::linearRand(1.0f, 2.5f);
			float rightFlatnessFactor = glm::linearRand(1, 3);

			int stemSegmentCount = 0;
			for(int i = 1; i < spline->Nodes.size(); i++)
			{
				auto& prev = spline->Nodes.at(i - 1);
				auto& curr = spline->Nodes.at(i);
				if(i == stemNodeCount)
				{
					stemSegmentCount = spline->Segments.size();
				}
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
						prev.Theta * (1.0f - div) + curr.Theta * div,
						glm::sin(temp * leftPeriodFactor) * leftFlatness,
						glm::sin(temp * rightPeriodFactor) * rightFlatness,
						leftFlatnessFactor, rightFlatnessFactor);
					temp += 0.3f;
				}
			}
			
			//Truck
			const int step = 5;

			const int vertexIndex = spline->Vertices.size();
			Vertex archetype;
			const float xStep = 1.0f / step / 2.0f;
			const float yStemStep = 0.5f / static_cast<float>(stemSegmentCount);
			const float yLeafStep = 0.5f / (spline->Segments.size() - static_cast<float>(stemSegmentCount) + 1);
			for (int i = 0; i < spline->Segments.size(); i++)
			{
				auto& segment = spline->Segments.at(i);
				const float angleStep = segment.Theta / step;
				const int vertsCount = step * 2 + 1;
				for (int j = 0; j < vertsCount; j++)
				{
					archetype.Position = segment.GetPoint((j - step) * angleStep);
					float yPos = (i < stemSegmentCount) ? yStemStep * i : 0.5f + yLeafStep * (i - stemSegmentCount + 1);
					archetype.TexCoords0 = glm::vec2(j * xStep, yPos);
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
	std::vector<Entity> plants;
	_PlantQuery.ToEntityArray(plants);
	for (auto& plant : plants) {
		auto pmmc = EntityManager::GetSharedComponent<MeshRenderer>(plant);
		auto spline = EntityManager::GetSharedComponent<Spline>(plant);
		pmmc->Mesh->SetVertices(17, spline->Vertices, spline->Indices, true);
		EntityManager::ForEachChild(plant, [](Entity child)
			{
				auto mmc = EntityManager::GetSharedComponent<MeshRenderer>(child);
				auto childSpline = EntityManager::GetSharedComponent<Spline>(child);
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

	for (auto& curve : truckSpline->Curves) {
		curve.CP0 -= posSum;
		curve.CP1 -= posSum;
		curve.CP2 -= posSum;
		curve.CP3 -= posSum;
	}
	truckSpline->Left = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), truckSpline->Curves.begin()->CP0 - truckSpline->Curves.back().CP3);
	for (int i = 0; i < leafCount; i++) {
		Entity leaf = CreateLeafForPlant(truck);
		auto leafSpline = EntityManager::GetSharedComponent<Spline>(leaf);
		float startingPoint;
		file >> startingPoint;
		
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

		auto mesh = EntityManager::GetSharedComponent<MeshRenderer>(plant)->Mesh;
		ObjExportHelper(glm::vec3(0.0f), mesh, of, startIndex);
		
		EntityManager::ForEachChild(plant, [this, &of, &startIndex](Entity child)
			{
				auto lt = EntityManager::GetComponentData<LocalTranslation>(child).Value;
				auto mesh = EntityManager::GetSharedComponent<MeshRenderer>(child)->Mesh;
				ObjExportHelper(lt, mesh, of, startIndex);
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
	_StemMaterial = std::make_shared<Material>();
	_StemMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseTruck->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	_StemMaterial->Textures2Ds()->push_back(textureDiffuseTruck);

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureLeaf = new Texture2D(TextureType::DIFFUSE);
	textureLeaf->LoadTexture("../Resources/Textures/leafSurface2.jpg", "");
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
