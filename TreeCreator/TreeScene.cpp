#include "TreeScene.h"

void TreeScene::ExportSceneAsOBJ(std::string filename)
{
	std::ofstream of;
	of.open((filename + ".obj").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		auto treeQuery = TreeManager::GetTreeQuery();
		std::vector<Entity> trees;
		treeQuery.ToEntityArray(trees);
		std::string start = "# Tree Collection, by Bosheng Li";
		start += "\n";
		of.write(start.c_str(), start.size());
		of.flush();
		unsigned startIndex = 1;
		for (auto treeEntity : trees)
		{
			auto ltw = EntityManager::GetComponentData<GlobalTransform>(treeEntity);
			auto mesh = EntityManager::GetPrivateComponent<MeshRenderer>(treeEntity)->Mesh;
			if(!mesh->GetVerticesUnsafe().empty() && !mesh->GetIndicesUnsafe().empty())
			{
				std::string header = "#Tree vertices: " + std::to_string(mesh->GetVerticesUnsafe().size()) +", tris: " + std::to_string(mesh->GetIndicesUnsafe().size() / 3);
				header += "\n";
				of.write(header.c_str(), header.size());
				of.flush();
				std::string o = "o ";
				o += 
					"["
				+ std::to_string(ltw.Value[3].x) + ","
				+ std::to_string(ltw.Value[3].z)
				+ "]" + "\n";
				of.write(o.c_str(), o.size());
				of.flush();
				std::string data;
#pragma region Data collection
				
				for (const auto& vertex : mesh->GetVerticesUnsafe()) {
					data += "v " + std::to_string(vertex.Position.x + ltw.Value[3].x)
						+ " " + std::to_string(-vertex.Position.z + ltw.Value[3].y)
						+ " " + std::to_string(vertex.Position.y + ltw.Value[3].z)
						+ "\n";
				}

				//data += "s off\n";
				data += "# List of indices for faces vertices, with (x, y, z).\n";
				for (auto i = 0; i < mesh->GetIndicesUnsafe().size() / 3; i++) {
					auto f1 = mesh->GetIndicesUnsafe().at(3l * i) + startIndex;
					auto f2 = mesh->GetIndicesUnsafe().at(3l * i + 1) + startIndex;
					auto f3 = mesh->GetIndicesUnsafe().at(3l * i + 2) + startIndex;
					data += "f " + std::to_string(f1)
						+ " " + std::to_string(f2)
						+ " " + std::to_string(f3)
						+ "\n";
				}
				startIndex += mesh->GetVerticesUnsafe().size();
#pragma endregion
				of.write(data.c_str(), data.size());
				of.flush();
				
			}
		}

		of.close();
		Debug::Log("Scene saved as " + filename + ".obj");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}
