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
	if (!mesh->GetVerticesUnsafe().empty() && !mesh->GetIndicesUnsafe().empty())
	{
		std::string header = "#Vertices: " + std::to_string(mesh->GetVerticesUnsafe().size()) + ", tris: " + std::to_string(mesh->GetIndicesUnsafe().size() / 3);
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

		for (const auto& vertex : mesh->GetVerticesUnsafe()) {
			data += "v " + std::to_string(vertex.Position.x + position.x)
				+ " " + std::to_string(vertex.Position.z + position.z)
				+ " " + std::to_string(vertex.Position.y + position.y)
				+ "\n";
		}
		for (const auto& vertex : mesh->GetVerticesUnsafe()) {
			data += "vn " + std::to_string(vertex.Normal.x)
				+ " " + std::to_string(vertex.Normal.z)
				+ " " + std::to_string(vertex.Normal.y)
				+ "\n";
		}
		
		for (const auto& vertex : mesh->GetVerticesUnsafe()) {
			data += "vt " + std::to_string(vertex.TexCoords0.x)
				+ " " + std::to_string(vertex.TexCoords0.y)
				+ "\n";
		}
		//data += "s off\n";
		data += "# List of indices for faces vertices, with (x, y, z).\n";
		for (auto i = 0; i < mesh->GetIndicesUnsafe().size() / 3; i++) {
			auto f1 = mesh->GetIndicesUnsafe().at(3l * i) + startIndex;
			auto f2 = mesh->GetIndicesUnsafe().at(3l * i + 1) + startIndex;
			auto f3 = mesh->GetIndicesUnsafe().at(3l * i + 2) + startIndex;
			data += "f " + std::to_string(f1) + "/" + std::to_string(f1) + "/" + std::to_string(f1)
				+ " " + std::to_string(f2) + "/" + std::to_string(f2) + "/" + std::to_string(f2)
				+ " " + std::to_string(f3) + "/" + std::to_string(f3) + "/" + std::to_string(f3)
				+ "\n";
		}
		startIndex += mesh->GetVerticesUnsafe().size();
#pragma endregion
		of.write(data.c_str(), data.size());
		of.flush();
	}
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CopyPlant(Entity original)
{
	Entity plant = EntityManager::CreateEntity(_PlantArchetype);
	plant.SetComponentData(EntityManager::GetComponentData<SorghumInfo>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Translation>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Rotation>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Scale>(original));
	plant.SetSharedComponent(EntityManager::GetSharedComponent<Spline>(original));
	plant.SetSharedComponent(EntityManager::GetSharedComponent<MeshRenderer>(original));
	EntityManager::ForEachChild(original, [this, &plant](Entity child)
		{
			Entity newChild = EntityManager::CreateEntity(_LeafArchetype);
			newChild.SetComponentData(EntityManager::GetComponentData<LeafInfo>(child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalTranslation> (child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalRotation>(child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalScale>(child));
			newChild.SetSharedComponent(EntityManager::GetSharedComponent<Spline>(child));
			newChild.SetSharedComponent(EntityManager::GetSharedComponent<MeshRenderer>(child));
			EntityManager::SetParent(newChild, plant);
		});
	return plant;
}

Entity SorghumReconstruction::SorghumReconstructionSystem::CreateGridPlant(Entity original, std::vector<glm::mat4>& matrices, bool rotateLeaves, float devAngle)
{
	Entity plant = EntityManager::CreateEntity(_PlantArchetype);
	plant.SetComponentData(EntityManager::GetComponentData<SorghumInfo>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Translation>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Rotation>(original));
	plant.SetComponentData(EntityManager::GetComponentData<Scale>(original));
	plant.SetSharedComponent(EntityManager::GetSharedComponent<Spline>(original));
	auto imr = std::make_shared<InstancedMeshRenderer>();
	const auto mr = EntityManager::GetSharedComponent<MeshRenderer>(original);
	imr->Mesh = mr->Mesh;
	imr->BackCulling = false;
	imr->Material = _InstancedStemMaterial;
	imr->Matrices.clear();
	imr->Matrices.insert(imr->Matrices.begin(), matrices.begin(), matrices.end());
	imr->RecalculateBoundingBox();
	plant.SetSharedComponent(imr);
	EntityManager::ForEachChild(original, [this, &plant, &matrices](Entity child)
		{
			Entity newChild = EntityManager::CreateEntity(_LeafArchetype);
			newChild.SetComponentData(EntityManager::GetComponentData<LeafInfo>(child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalTranslation>(child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalRotation>(child));
			newChild.SetComponentData(EntityManager::GetComponentData<LocalScale>(child));
			newChild.SetSharedComponent(EntityManager::GetSharedComponent<Spline>(child));
			auto imr = std::make_shared<InstancedMeshRenderer>();
			const auto mr = EntityManager::GetSharedComponent<MeshRenderer>(child);
			imr->Mesh = mr->Mesh;
			imr->BackCulling = false;
			imr->Material = _InstancedLeafMaterial;
			imr->Matrices.clear();
			imr->Matrices.insert(imr->Matrices.begin(), matrices.begin(), matrices.end());
			imr->RecalculateBoundingBox();
			newChild.SetSharedComponent(imr);
			EntityManager::SetParent(newChild, plant);
		});
	return plant;
}


void SorghumReconstruction::SorghumReconstructionSystem::GenerateMeshForAllPlants(int segmentAmount, int step)
{
	std::mutex meshMutex;
	EntityManager::ForEach<LocalToWorld>(_SplineQuery, [&meshMutex, segmentAmount, step]
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
					spline->Nodes.emplace_back(truckSpline->EvaluatePointFromCurve(i), 180.0f, width, truckSpline->EvaluateAxisFromCurve(i), false);

				}
				stemNodeCount = spline->Nodes.size();
				for (float i = 0.05f; i <= 1.0f; i += 0.05f)
				{
					float w = 0.2f;
					if (i > 0.75f) w -= (i - 0.75f) * 0.75f;
					spline->Nodes.emplace_back(spline->EvaluatePointFromCurve(i), i == 0.05f ? 60.0f : 10.0f, w, spline->EvaluateAxisFromCurve(i), true);
				}
			}else
			{
				for (float i = 0.0f; i <= 1.0f; i += 0.05f)
				{
					spline->Nodes.emplace_back(spline->EvaluatePointFromCurve(i), 180.0f, 0.04f, spline->EvaluateAxisFromCurve(i), false);
				}
				auto endPoint = spline->EvaluatePointFromCurve(1.0f);
				auto endAxis = spline->EvaluateAxisFromCurve(1.0f);
				spline->Nodes.emplace_back(endPoint + endAxis * 0.05f, 10.0f, 0.001f, endAxis, false);
				stemNodeCount = spline->Nodes.size();
			}
			spline->Vertices.clear();
			spline->Indices.clear();
			spline->Segments.clear();


			float temp = 0.0f;

			float leftPeriod = 0.0f;
			float rightPeriod = 0.0f;
			float leftFlatness = glm::gaussRand(1.75f, 0.5f);//glm::linearRand(0.5f, 2.0f);
			float rightFlatness = glm::gaussRand(1.75f, 0.5f);//glm::linearRand(0.5f, 2.0f);
			float leftFlatnessFactor = glm::gaussRand(1.25f, 0.2f);//glm::linearRand(1.0f, 2.5f);
			float rightFlatnessFactor = glm::gaussRand(1.25f, 0.2f);//glm::linearRand(1.0f, 2.5f);

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
				for(float div = 1.0f / segmentAmount; div <= 1.0f; div += 1.0f / segmentAmount)
				{
					auto front = prev.Axis * (1.0f - div) + curr.Axis * div;
					
					auto up = glm::normalize(glm::cross(spline->Left, front));
					if (prev.IsLeaf) {
						leftPeriod += glm::gaussRand(1.25f, 0.5f) / segmentAmount;
						rightPeriod += glm::gaussRand(1.25f, 0.5f) / segmentAmount;
						spline->Segments.emplace_back(
							curve.GetPoint(div),
							up,
							front,
							prev.Width * (1.0f - div) + curr.Width * div,
							prev.Theta * (1.0f - div) + curr.Theta * div,
							curr.IsLeaf,
							glm::sin(leftPeriod) * leftFlatness,
							glm::sin(rightPeriod) * rightFlatness,
							leftFlatnessFactor, rightFlatnessFactor);
					}else
					{
						spline->Segments.emplace_back(
							curve.GetPoint(div),
							up,
							front,
							prev.Width * (1.0f - div) + curr.Width * div,
							prev.Theta * (1.0f - div) + curr.Theta * div,
							curr.IsLeaf);
					}
				}
			}

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
	_StemMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), TextureType::DIFFUSE);
	_StemMaterial->SetTexture(textureDiffuseTruck);
	_StemMaterial->SetMaterialProperty("material.shininess", 1.0f);
	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	auto textureLeaf = AssetManager::LoadTexture("../Resources/Textures/leafSurfaceBright.jpg", TextureType::DIFFUSE);
	_LeafMaterial->SetTexture(textureLeaf);
	_LeafMaterial->SetMaterialProperty("material.shininess", 1.0f);
	_InstancedStemMaterial = std::make_shared<Material>();
	_InstancedStemMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_InstancedStemMaterial->SetTexture(textureDiffuseTruck);
	_InstancedStemMaterial->SetMaterialProperty("material.shininess", 0.5f);
	_InstancedLeafMaterial = std::make_shared<Material>();
	_InstancedLeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_InstancedLeafMaterial->SetTexture(textureLeaf);
	_InstancedLeafMaterial->SetMaterialProperty("material.shininess", 1.0f);
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
