#include "SorghumReconstructionSystem.h"


void SorghumRecon::SorghumReconstructionSystem::DeleteTruck(Entity& truckEntity)
{
	auto spline = EntityManager::GetComponentData<Spline>(truckEntity);
	delete spline.Curves;
	delete spline.Vertices;
	delete spline.Indices;
	auto rml = EntityManager::GetComponentData<RingMeshList>(truckEntity);
	delete rml.Rings;
	MeshMaterialComponent* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(truckEntity);
	if (mmc->_Mesh != nullptr) delete mmc->_Mesh;
	EntityManager::ForEachChild(truckEntity, [](Entity child) {
		auto childSpline = EntityManager::GetComponentData<Spline>(child);
		delete childSpline.Curves;
		delete childSpline.Vertices;
		delete childSpline.Indices;
		auto rml = EntityManager::GetComponentData<RingMeshList>(child);
		delete rml.Rings;
		MeshMaterialComponent* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(child);
		if (mmc->_Mesh != nullptr) delete mmc->_Mesh;
		});
	EntityManager::DeleteEntity(truckEntity);
}

Entity SorghumRecon::SorghumReconstructionSystem::CreateTruck()
{
	Entity ret = EntityManager::CreateEntity(_TruckArchetype);
	Scale s;
	s.Value = glm::vec3(1.0f);
	Spline spline;
	spline.Curves = new std::vector<BezierCurve>();
	spline.Vertices = new std::vector<Vertex>();
	spline.Indices = new std::vector<unsigned>();
	RingMeshList rml;
	rml.Rings = new std::vector<RingMesh>();
	EntityManager::SetComponentData(ret, rml);
	EntityManager::SetComponentData(ret, spline);
	EntityManager::SetComponentData(ret, s);
	MeshMaterialComponent* mmc = new MeshMaterialComponent();
	mmc->_Material = _TruckMaterial;
	mmc->_Mesh = nullptr;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

Entity SorghumRecon::SorghumReconstructionSystem::CreateLeafForTruck(Entity& truckEntity)
{
	Entity ret = EntityManager::CreateEntity(_LeafArchetype);
	EntityManager::SetParent(ret, truckEntity);
	LocalScale ls;
	ls.Value = glm::vec3(1.0f);
	Spline spline;
	spline.Curves = new std::vector<BezierCurve>();
	spline.Vertices = new std::vector<Vertex>();
	spline.Indices = new std::vector<unsigned>();
	RingMeshList rml;
	rml.Rings = new std::vector<RingMesh>();
	EntityManager::SetComponentData(ret, rml);
	EntityManager::SetComponentData(ret, spline);
	EntityManager::SetComponentData(ret, ls);

	MeshMaterialComponent* mmc = new MeshMaterialComponent();
	mmc->_Material = _LeafMaterial;
	mmc->_Mesh = nullptr;
	EntityManager::SetSharedComponent(ret, mmc);

	return ret;
}

Entity SorghumRecon::SorghumReconstructionSystem::CreatePlant(std::string path, float resolution)
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
	Spline truckSpline = EntityManager::GetComponentData<Spline>(truck);
	


	truckSpline.StartingPoint = -1;
	truckSpline.Import(file);

	//Recenter plant:
	glm::vec3 posSum = glm::vec3(0.0f);
	int pointAmount = 0;
	for (auto& curve : *truckSpline.Curves) {
		pointAmount += 2;
		posSum += curve.CP0;
		posSum += curve.CP3;
	}
	posSum /= pointAmount;
	for (auto& curve : *truckSpline.Curves) {
		curve.CP0 -= posSum;
		curve.CP1 -= posSum;
		curve.CP2 -= posSum;
		curve.CP3 -= posSum;
	}

	EntityManager::SetComponentData(truck, truckSpline);
	for (int i = 0; i < leafCount; i++) {
		Entity leaf = CreateLeafForTruck(truck);
		Spline leafSpline = EntityManager::GetComponentData<Spline>(leaf);
		float startingPoint;
		file >> startingPoint;
		leafSpline.StartingPoint = startingPoint;
		leafSpline.Import(file);
		EntityManager::SetComponentData(leaf, leafSpline);

		const float splineU = glm::clamp(startingPoint, 0.0f, 1.0f) * float(truckSpline.Curves->size());

		// Decompose the global u coordinate on the spline
		float integerPart;
		const float fractionalPart = modff(splineU, &integerPart);

		auto curveIndex = int(integerPart);
		auto curveU = fractionalPart;

		// If evaluating the very last point on the spline
		if (curveIndex == truckSpline.Curves->size() && curveU <= 0.0f)
		{
			// Flip to the end of the last patch
			curveIndex--;
			curveU = 1.0f;
		}

		LocalTranslation lt;
		lt.Value = truckSpline.Curves->at(curveIndex).Evaluate(curveU);
		EntityManager::SetComponentData(leaf, lt);
	}

	EntityManager::ForEach<Spline, RingMeshList>(_SplineQuery, [resolution]
	(int index, Entity entity, Spline* spline, RingMeshList* rml)
		{
			spline->Vertices->clear();
			spline->Indices->clear();

			auto rings = rml->Rings;
			rings->clear();
			for (auto& curve : *spline->Curves) {
				int amount = 10;


				glm::vec3 fromDir = curve.GetStartAxis();
				glm::vec3 dir = curve.GetEndAxis();
				float posStep = 1.0f / (float)amount;
				glm::vec3 dirStep = (dir - fromDir) / (float)amount;
				float thickness = 0.03f;
				float parentThickness = 0.03f;
				float radiusStep = 0;
				for (int i = 1; i < amount; i++) {
					rings->push_back(RingMesh(
						curve.GetPoint(posStep * (i - 1)), curve.GetPoint(posStep * i),
						fromDir + (float)(i - 1) * dirStep, fromDir + (float)i * dirStep,
						parentThickness + (float)(i - 1) * radiusStep, parentThickness + (float)i * radiusStep));
				}
				if (amount > 1)rings->push_back(RingMesh(curve.GetPoint(1.0f - posStep), curve.CP3, dir - dirStep, dir, thickness - radiusStep, thickness));
				else rings->push_back(RingMesh(curve.CP0, curve.CP3, fromDir, dir, parentThickness, thickness));
			}

			if (spline->StartingPoint == -1) {
				//Truck
				int step = 12;
				float angleStep = 360.0f / (float)(step);
				glm::vec3 newNormalDir = glm::vec3(0, 1, 0);
				int vertexIndex = spline->Vertices->size();
				Vertex archetype;
				float textureXstep = 1.0f / step * 4.0f;
				for (int i = 0; i < step; i++) {
					archetype.Position = rings->at(0).GetPoint(newNormalDir, angleStep * i, true);
					float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
					archetype.TexCoords0 = glm::vec2(x, 0.0f);
					spline->Vertices->push_back(archetype);
				}
				int ringSize = rings->size();
				float textureYstep = 1.0f / ringSize * 2.0f;
				for (int ringIndex = 0; ringIndex < ringSize; ringIndex++) {
					for (int i = 0; i < step; i++) {
						archetype.Position = rings->at(ringIndex).GetPoint(newNormalDir, angleStep * i, false);
						float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
						float y = ringIndex < (ringSize / 2) ? (ringIndex + 1) * textureYstep : (ringSize - ringIndex - 1) * textureYstep;
						archetype.TexCoords0 = glm::vec2(x, y);
						spline->Vertices->push_back(archetype);
					}
					for (int i = 0; i < step - 1; i++) {
						//Down triangle
						spline->Indices->push_back(vertexIndex + ringIndex * step + i);
						spline->Indices->push_back(vertexIndex + ringIndex * step + i + 1);
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i);
						//Up triangle
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i + 1);
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i);
						spline->Indices->push_back(vertexIndex + ringIndex * step + i + 1);
					}
					//Down triangle
					spline->Indices->push_back(vertexIndex + ringIndex * step + step - 1);
					spline->Indices->push_back(vertexIndex + ringIndex * step);
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
					//Up triangle
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step);
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
					spline->Indices->push_back(vertexIndex + ringIndex * step);
				}
			}
			else {
				//Leaf
				int step = 12;
				float angleStep = 360.0f / (float)(step);
				glm::vec3 newNormalDir = glm::vec3(0, 1, 0);
				int vertexIndex = spline->Vertices->size();
				Vertex archetype;
				float textureXstep = 1.0f / step * 4.0f;
				for (int i = 0; i < step; i++) {
					archetype.Position = rings->at(0).GetPoint(newNormalDir, angleStep * i, true);
					float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
					archetype.TexCoords0 = glm::vec2(x, 0.0f);
					spline->Vertices->push_back(archetype);
				}
				int ringSize = rings->size();
				float textureYstep = 1.0f / ringSize * 2.0f;
				for (int ringIndex = 0; ringIndex < ringSize; ringIndex++) {
					for (int i = 0; i < step; i++) {
						archetype.Position = rings->at(ringIndex).GetPoint(newNormalDir, angleStep * i, false);
						float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
						float y = ringIndex < (ringSize / 2) ? (ringIndex + 1) * textureYstep : (ringSize - ringIndex - 1) * textureYstep;
						archetype.TexCoords0 = glm::vec2(x, y);
						spline->Vertices->push_back(archetype);
					}
					for (int i = 0; i < step - 1; i++) {
						//Down triangle
						spline->Indices->push_back(vertexIndex + ringIndex * step + i);
						spline->Indices->push_back(vertexIndex + ringIndex * step + i + 1);
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i);
						//Up triangle
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i + 1);
						spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + i);
						spline->Indices->push_back(vertexIndex + ringIndex * step + i + 1);
					}
					//Down triangle
					spline->Indices->push_back(vertexIndex + ringIndex * step + step - 1);
					spline->Indices->push_back(vertexIndex + ringIndex * step);
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
					//Up triangle
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step);
					spline->Indices->push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
					spline->Indices->push_back(vertexIndex + ringIndex * step);
				}
			}
		}
	);
	MeshMaterialComponent* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(truck);
	truckSpline = EntityManager::GetComponentData<Spline>(truck);
	if (mmc->_Mesh != nullptr) delete mmc->_Mesh;
	mmc->_Mesh = new Mesh();
	mmc->_Mesh->SetVertices(17, *truckSpline.Vertices, *truckSpline.Indices);
	EntityManager::SetComponentData(truck, truckSpline);

	EntityManager::ForEachChild(truck, [](Entity child) 
		{
			MeshMaterialComponent* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(child);
			auto childSpline = EntityManager::GetComponentData<Spline>(child);
			if (mmc->_Mesh != nullptr) delete mmc->_Mesh;
			mmc->_Mesh = new Mesh();
			mmc->_Mesh->SetVertices(17, *childSpline.Vertices, *childSpline.Indices);
			EntityManager::SetComponentData(child, childSpline);
		}
	);
	return truck;
}

void SorghumRecon::SorghumReconstructionSystem::OnCreate()
{
	_TruckArchetype = EntityManager::CreateEntityArchetype("Truck",
		TruckInfo(), Spline(), RingMeshList(),
		Translation(), Rotation(), Scale(), LocalToWorld(), MeshMaterialComponent()
	);
	_LeafArchetype = EntityManager::CreateEntityArchetype("Leaf",
		LeafInfo(), Spline(), RingMeshList(),
		LocalTranslation(), LocalRotation(), LocalScale(), LocalToParent(), LocalToWorld(), MeshMaterialComponent()
	);
	_SplineQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_SplineQuery, Spline());

	_TruckMaterial = new Material();
	_TruckMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuseTruck = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseTruck->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	auto textureNormalTruck = new Texture2D(TextureType::NORMAL);
	textureNormalTruck->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_normal.jpg"), "");
	_TruckMaterial->Textures2Ds()->push_back(textureDiffuseTruck);
	_TruckMaterial->Textures2Ds()->push_back(textureNormalTruck);

	_LeafMaterial = new Material();
	_LeafMaterial->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureLeaf = new Texture2D(TextureType::DIFFUSE);
	textureLeaf->LoadTexture(FileIO::GetResourcePath("Textures/green.png"), "");
	auto textureNormalLeaf = new Texture2D(TextureType::NORMAL);
	textureNormalLeaf->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"), "");
	_LeafMaterial->Textures2Ds()->push_back(textureLeaf);
	_LeafMaterial->Textures2Ds()->push_back(textureNormalLeaf);
	Enable();
}

void SorghumRecon::SorghumReconstructionSystem::OnDestroy()
{
}

void SorghumRecon::SorghumReconstructionSystem::Update()
{

}

void SorghumRecon::SorghumReconstructionSystem::FixedUpdate()
{
}

void SorghumRecon::Spline::Import(std::ifstream& stream)
{
	int curveAmount;
	stream >> curveAmount;
	for (int i = 0; i < curveAmount; i++) {
		glm::vec3 cp[4];
		float x, y, z;
		for (int j = 0; j < 4; j++) {
			stream >> x >> y >> z;
			cp[j] = glm::vec3(x, y, z) * 10.0f;
		}
		Curves->push_back(BezierCurve(cp[0], cp[1], cp[2], cp[3]));
	}
}
