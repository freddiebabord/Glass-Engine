#include "PCH/stdafx.h"
#include "VRSystem.h"
#include "Application.h"

namespace GlassEngine::Internal
{
	VRSystem::VRSystem():
		vrSystem(nullptr),
		renderModels(nullptr),
		m_nRenderTargetWidth(0),
		m_nRenderTargetHeight(0), 
		appSettings(nullptr)
	{
	}

	VRSystem::~VRSystem()
	{
		if(vrSystem)
			vr::VR_Shutdown();
		vrSystem = nullptr;
	}

	void VRSystem::Init()
	{
		appSettings = ApplicationSettings::Instance();
		enableVR = appSettings->graphics.isVRApplciation;
		vr::HmdError error;
		vrSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
		if (error != vr::VRInitError_None)
		{
			printf("VR :: Unable to init VR\n");
			return;
		}

		renderModels = static_cast<vr::IVRRenderModels *>(vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &error));
		if (!renderModels)
		{
			vrSystem = nullptr;
			vr::VR_Shutdown();
			fprintf(stderr, "VR :: Unable to get render model interface: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(error));
			return;
		}

		if (!vr::VRCompositor())
		{
			fprintf(stderr, "VR :: Compositor initialization failed. See log file for details\n");
			return;
		}

		vrSystem->GetRecommendedRenderTargetSize(&m_nRenderTargetWidth, &m_nRenderTargetHeight);
		SetupCameras();

		printf("VR :: Initialized successfully.\n");
	}

	void VRSystem::Shutdown()
	{
		if (vrSystem)
			vr::VR_Shutdown();
		vrSystem = nullptr;
		enableVR = false;
	}

	bool VRSystem::VRSystemAvailable()
	{
		if(!vr::VR_IsHmdPresent())
			return false;
		if (!vr::VR_IsRuntimeInstalled())
			return false;
		return true;
	}
		
	bool VRSystem::GetVulkanInstanceExtensionsRequired(std::vector< std::string > &outInstanceExtensionList)
	{
		if (!vr::VRCompositor())
		{
			return false;
		}

		outInstanceExtensionList.clear();
		uint32_t nBufferSize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(nullptr, 0);
		if (nBufferSize > 0)
		{
			// Allocate memory for the space separated list and query for it
			char *pExtensionStr = new char[nBufferSize];
			pExtensionStr[0] = 0;
			vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(pExtensionStr, nBufferSize);

			// Break up the space separated list into entries on the CUtlStringList
			std::string curExtStr;
			uint32_t nIndex = 0;
			while (pExtensionStr[nIndex] != 0 && (nIndex < nBufferSize))
			{
				if (pExtensionStr[nIndex] == ' ')
				{
					outInstanceExtensionList.push_back(curExtStr);
					curExtStr.clear();
				}
				else
				{
					curExtStr += pExtensionStr[nIndex];
				}
				nIndex++;
			}
			if (curExtStr.size() > 0)
			{
				outInstanceExtensionList.push_back(curExtStr);
			}

			delete[] pExtensionStr;
		}

		return true;
	}
	
	bool VRSystem::GetVulkanDeviceExtensionsRequired(VkPhysicalDevice pPhysicalDevice, std::vector< std::string > &outDeviceExtensionList)
	{
		if (!vr::VRCompositor())
		{
			return false;
		}

		outDeviceExtensionList.clear();
		uint32_t nBufferSize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired((VkPhysicalDevice_T *)pPhysicalDevice, nullptr, 0);
		if (nBufferSize > 0)
		{
			// Allocate memory for the space separated list and query for it
			char *pExtensionStr = new char[nBufferSize];
			pExtensionStr[0] = 0;
			vr::VRCompositor()->GetVulkanDeviceExtensionsRequired((VkPhysicalDevice_T *)pPhysicalDevice, pExtensionStr, nBufferSize);

			// Break up the space separated list into entries on the CUtlStringList
			std::string curExtStr;
			uint32_t nIndex = 0;
			while (pExtensionStr[nIndex] != 0 && (nIndex < nBufferSize))
			{
				if (pExtensionStr[nIndex] == ' ')
				{
					outDeviceExtensionList.push_back(curExtStr);
					curExtStr.clear();
				}
				else
				{
					curExtStr += pExtensionStr[nIndex];
				}
				nIndex++;
			}
			if (curExtStr.size() > 0)
			{
				outDeviceExtensionList.push_back(curExtStr);
			}

			delete[] pExtensionStr;
		}

		return true;
	}

	void VRSystem::UpdateHMDMatrixPose()
	{
		if (!vrSystem || !enableVR)
			return;

		vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

		m_iValidPoseCount = 0;
		m_strPoseClasses = "";
		for (uint32_t nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
		{
			if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
			{
				m_iValidPoseCount++;
				m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
				if (m_rDevClassChar[nDevice] == 0)
				{
					switch (vrSystem->GetTrackedDeviceClass(nDevice))
					{
					case vr::TrackedDeviceClass_Controller: m_rDevClassChar[nDevice] = 'C';
						break;
					case vr::TrackedDeviceClass_HMD: m_rDevClassChar[nDevice] = 'H';
						break;
					case vr::TrackedDeviceClass_Invalid: m_rDevClassChar[nDevice] = 'I';
						break;
					case vr::TrackedDeviceClass_GenericTracker: m_rDevClassChar[nDevice] = 'G';
						break;
					case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T';
						break;
					default: m_rDevClassChar[nDevice] = '?';
						break;
					}
				}
				m_strPoseClasses += m_rDevClassChar[nDevice];
			}
		}

		if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
			m_mat4HMDPose = glm::inverse(m_mat4HMDPose);
		}

		// TODO: Add controller tracking
	}

	glm::mat4x4 VRSystem::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) const
	{
		if (!vrSystem)
			return glm::mat4x4(1.0);

		vr::HmdMatrix34_t matEyeRight = vrSystem->GetEyeToHeadTransform(nEye);
		glm::mat4x4 matrixObj(
			matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
			matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
			matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
			matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);

		return glm::inverse(matrixObj);
	}

	glm::mat4x4 VRSystem::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye) const
	{
		glm::mat4x4 matMVP;
		if (nEye == vr::Eye_Left)
		{
			matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
		}
		else if (nEye == vr::Eye_Right)
		{
			matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose;
		}

		return matMVP;
	}

	glm::mat4x4 VRSystem::GetCurrentViewMatrix(vr::Hmd_Eye nEye) const
	{
		glm::mat4x4 matMVP;
		if (nEye == vr::Eye_Left)
		{
			matMVP = m_mat4eyePosLeft * m_mat4HMDPose;
		}
		else if (nEye == vr::Eye_Right)
		{
			matMVP = m_mat4eyePosRight *  m_mat4HMDPose;
		}

		return matMVP;
	}

	glm::mat4x4 VRSystem::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) const
	{
		if (!vrSystem)
			return glm::mat4x4();

		vr::HmdMatrix44_t mat = vrSystem->GetProjectionMatrix(nEye, appSettings->camera.nearClipPlane, appSettings->camera.farClipPlane);

		// NOTE: Flip y for Vulkan
		return glm::mat4x4(
			mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
			mat.m[0][1],-mat.m[1][1], mat.m[2][1], mat.m[3][1],
			mat.m[0][2], mat.m[1][2], mat.m[2][2]/2, mat.m[3][2],
			mat.m[0][3], mat.m[1][3], mat.m[2][3]/2, mat.m[3][3]
		);
	}

	glm::mat4x4 VRSystem::GetHMDPose() const
	{
		return m_mat4HMDPose;
	}

	void VRSystem::SetupCameras()
	{
		m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
		m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
		m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
		m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
	}

	uint32_t VRSystem::GetRenderWidth() const
	{
		return m_nRenderTargetWidth;
	}

	uint32_t VRSystem::GetRenderHeight() const
	{
		return m_nRenderTargetHeight;
	}

	bool VRSystem::EnableVR() const
	{
		return enableVR;
	}

	glm::mat4x4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
	{
		glm::mat4x4 matrixObj(
			matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
			matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
			matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
			matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
		return matrixObj;
	}
}
