#include "LightEstimator.h"
#include "TreeManager.h"
TreeUtilities::LightSnapShot::LightSnapShot(size_t resolution, glm::vec3 centerPosition, glm::vec3 direction, float centerDistance, float width, float weight)
{
	_SnapShotTexture = std::make_unique<GLTexture2D>(1, GL_R32F, resolution, resolution);
	_SnapShotTexture->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_SnapShotTexture->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_SnapShotTexture->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_SnapShotTexture->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	_Direction = glm::normalize(direction);
	_CenterPosition = centerPosition;
	_CenterDistance = centerDistance;
	_Weight = weight;
	_Width = width;
	_Score = 0;
	_Resolution = resolution;
	_SRC.resize(resolution * resolution);
}

glm::mat4 TreeUtilities::LightSnapShot::GetViewMatrix()
{
	glm::mat4 view = glm::lookAt(_CenterPosition - _CenterDistance * _Direction, _CenterPosition, glm::vec3(0, 1, 0));
	if (glm::any(glm::isnan(view[3]))) {
		view = glm::lookAt(_CenterPosition - _CenterDistance * _Direction, _CenterPosition, glm::vec3(0, 0, 1));
	}
	return view;
}

glm::mat4 TreeUtilities::LightSnapShot::GetLightSpaceMatrix()
{
	glm::mat4 view = glm::lookAt(_CenterPosition - _CenterDistance * _Direction, _CenterPosition, glm::vec3(0, 1, 0));
	if (glm::any(glm::isnan(view[3]))) {
		view = glm::lookAt(_CenterPosition - _CenterDistance * _Direction, _CenterPosition, glm::vec3(0, 0, 1));
	}
	glm::mat4 projection;
	projection = glm::ortho(-_Width, _Width, -_Width, _Width, 0.0f, _CenterDistance * 2.0f);
	return projection * view;
}

glm::vec3 TreeUtilities::LightSnapShot::GetDirection()
{
	return _Direction;
}

float TreeUtilities::LightSnapShot::CalculateScore()
{
	size_t amount = 0;
	for (int i = 0; i < _Resolution * _Resolution; i++) {
		if (_SRC[4 * i] != 0.0f) {
			amount++;
		}
	}
	_Score = (float)amount / (float)_Resolution / (float)_Resolution * _Weight;
	return _Score;
}

float TreeUtilities::LightSnapShot::CenterDistance()
{
	return _CenterDistance;
}

float TreeUtilities::LightSnapShot::Width()
{
	return _Width;
}

float TreeUtilities::LightSnapShot::Weight()
{
	return _Weight;
}

float TreeUtilities::LightSnapShot::Resolution()
{
	return _Resolution;
}


unsigned TreeUtilities::LightSnapShot::GetEntityIndex(size_t x, size_t y)
{
	float r = (_SRC[(x * _Resolution + y)] + 0.5f);
	int ru = r;
	return ru;
}

void TreeUtilities::LightEstimator::SetMaxIllumination(float value)
{
	_MaxIllumination = value;
}

glm::vec3 TreeUtilities::LightEstimator::GetCenterPosition()
{
	return _CenterPositon;
}

TreeUtilities::LightEstimator::LightEstimator(size_t resolution, float centerDistance) : _Resolution(resolution)
{
	_RenderTarget = new RenderTarget(resolution, resolution);
	_DepthBuffer = new GLRenderBuffer();
	_DepthBuffer->AllocateStorage(GL_DEPTH_COMPONENT32, resolution, resolution);
	_RenderTarget->AttachRenderBuffer(_DepthBuffer, GL_DEPTH_ATTACHMENT);

	std::string vertShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString("../Resources/Shaders/TreeUtilities/LightSnapShot.vert");
	std::string fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString("../Resources/Shaders/TreeUtilities/LightSnapShot.frag");

	_SnapShotProgram = std::make_unique<GLProgram>(
		std::make_shared<GLShader>(ShaderType::Vertex, &vertShaderCode),
		std::make_shared<GLShader>(ShaderType::Fragment, &fragShaderCode));
}

void TreeUtilities::LightEstimator::ResetResolution(size_t value)
{
	_Resolution = value;
}

void TreeUtilities::LightEstimator::ResetCenterPosition(glm::vec3 position)
{
	_CenterPositon = position;
	for (const auto& ss : _SnapShots) {
		ss->_CenterPosition = position;
	}
}

void TreeUtilities::LightEstimator::ResetCenterDistance(float distance)
{
	_CenterDistance = distance;
	for (const auto& ss : _SnapShots) {
		ss->_CenterDistance = distance;
	}
}

void TreeUtilities::LightEstimator::ResetSnapShotWidth(float width)
{
	_SnapShotWidth = width;
	for (const auto& ss : _SnapShots) {
		ss->_Width = width;
	}
}

void TreeUtilities::LightEstimator::PushSnapShot(glm::vec3 direction, float weight)
{
	_SnapShots.push_back(new LightSnapShot(_Resolution, _CenterPositon, direction, _CenterDistance, _SnapShotWidth, weight));
}

void TreeUtilities::LightEstimator::Clear()
{
	for (int i = 0; i < _SnapShots.size(); i++) {
		delete _SnapShots[i];
	}
	_SnapShots.clear();
}

void TreeUtilities::LightEstimator::TakeSnapShot(bool storeSnapshot)
{
	std::vector<GlobalTransform> matrices = std::vector<GlobalTransform>();
	std::vector<Entity> internodeEntities = std::vector<Entity>();
	
	TreeManager::GetInternodeQuery().ToComponentDataArray(matrices);
	TreeManager::GetInternodeQuery().ToEntityArray(internodeEntities);
	auto mesh = Default::Primitives::Sphere;
	
	GLVBO indicesBuffer;
	size_t count = matrices.size();
	mesh->Enable();

	indicesBuffer.SetData((GLsizei)count * sizeof(Entity), internodeEntities.data(), GL_DYNAMIC_DRAW);
	mesh->VAO()->EnableAttributeArray(11);
	mesh->VAO()->SetAttributeIntPointer(11, 1, GL_UNSIGNED_INT, sizeof(Entity), (void*)0);
	mesh->VAO()->SetAttributeDivisor(11, 1);

	GLVBO matricesBuffer;
	matricesBuffer.SetData((GLsizei)count * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);

	mesh->VAO()->EnableAttributeArray(12);
	mesh->VAO()->SetAttributePointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	mesh->VAO()->EnableAttributeArray(13);
	mesh->VAO()->SetAttributePointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
	mesh->VAO()->EnableAttributeArray(14);
	mesh->VAO()->SetAttributePointer(14, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	mesh->VAO()->EnableAttributeArray(15);
	mesh->VAO()->SetAttributePointer(15, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
	mesh->VAO()->SetAttributeDivisor(12, 1);
	mesh->VAO()->SetAttributeDivisor(13, 1);
	mesh->VAO()->SetAttributeDivisor(14, 1);
	mesh->VAO()->SetAttributeDivisor(15, 1);

	glm::mat4 model;
	glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0f));
	glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f));
	model = translation * scale;
	_RenderTarget->Bind();
	_SnapShotProgram->Bind();
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	for (auto ss : _SnapShots) {
		auto texture = ss->SnapShotTexture().get();
		_RenderTarget->AttachTexture(texture, GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (count == 0) continue;
		_SnapShotProgram->SetFloat4x4("lightSpaceMatrix", ss->GetLightSpaceMatrix());
		_SnapShotProgram->SetFloat4x4("model", model);
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)mesh->Size(), GL_UNSIGNED_INT, 0, (GLsizei)count);
		if (storeSnapshot) {
			texture->Bind(0);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, ss->GetSRC().data());
		}
	}
	
	mesh->VAO()->DisableAttributeArray(11);
	mesh->VAO()->DisableAttributeArray(12);
	mesh->VAO()->DisableAttributeArray(13);
	mesh->VAO()->DisableAttributeArray(14);
	mesh->VAO()->DisableAttributeArray(15);	
	RenderTarget::BindDefault();
}

float TreeUtilities::LightEstimator::GetMaxIllumination()
{
	return _MaxIllumination;
}


float TreeUtilities::LightEstimator::CalculateScore()
{
	float currentScore, totalScore;
	currentScore = totalScore = 0;
	for (auto ss : _SnapShots) {
		float score = ss->CalculateScore();
		totalScore += score;
	}
	return totalScore;
}

std::vector<TreeUtilities::LightSnapShot*>* TreeUtilities::LightEstimator::GetSnapShots()
{
	return &_SnapShots;
}
