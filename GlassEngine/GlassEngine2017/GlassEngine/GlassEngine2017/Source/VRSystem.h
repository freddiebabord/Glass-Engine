#pragma once

#include <OpenVR/openvr.h>

namespace GlassEngine
{
	class ApplicationSettings;
}

namespace GlassEngine::Internal
{
	class VRSystem
	{
	public:
		VRSystem();
		~VRSystem();

		void Init();
		void Shutdown();

		static bool VRSystemAvailable();

		/// Ask OpenVR for the list of instance extensions required
		/// @param outInstanceExtensionList A vector to fill with Vulkan extensions OpenVR requires
		/// @return false if the vr compositor wasnt intialised
		static bool GetVulkanInstanceExtensionsRequired(std::vector<std::string>& outInstanceExtensionList);
		
		/// Ask OpenVR for the list of device extensions required
		/// @param pPhysicalDevice The physical device to check requirements against
		/// @param outDeviceExtensionList A vector to fill with Vulkan extensions OpenVR requires
		/// @return false if the vr compositor wasnt intialised
		static bool GetVulkanDeviceExtensionsRequired(VkPhysicalDevice pPhysicalDevice, std::vector< std::string > &outDeviceExtensionList);

		/// Update the 3D location of tracked VR objects
		void UpdateHMDMatrixPose();

		/// Gets an HMDMatrixPoseEye with respect to nEye.
		/// @param nEye Target eye
		/// @return local space matrix relative to VR bounds
		glm::mat4x4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) const;

		/// Gets a Current View Projection Matrix with respect to nEye
		/// @param nEye Target eye
		/// @return view projection matrix for target eye
		glm::mat4x4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye) const;

		/// Gets a Current View Matrix with respect to nEye
		/// @param nEye Target eye
		/// @return view matrix for target eye
		glm::mat4x4 GetCurrentViewMatrix(vr::Hmd_Eye nEye) const;

		/// Gets a Current Projection Matrix with respect to nEye
		/// @param nEye Target eye
		/// @return projection matrix for target eye
		glm::mat4x4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) const;

		/// Gets a Current transform matrix of the HMD
		/// @return view transform matrix of the HMD relative to Vr bounds
		glm::mat4x4 GetHMDPose() const;

		/// Get inital camera matricies otherwise vr system will crash
		void SetupCameras();

		/// Get the render width for an eye texture
		/// @return the width of a VR screen for a VR texture
		uint32_t GetRenderWidth() const;

		/// Get the render height for an eye texture
		/// @return the height of a VR screen for a VR texture
		uint32_t GetRenderHeight() const;

		/// Check to see if the VR subsyetm is enabled
		bool EnableVR() const;

	private:
		vr::IVRSystem* vrSystem;
		vr::IVRRenderModels* renderModels;
		uint32_t m_nRenderTargetWidth, m_nRenderTargetHeight;
		bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		glm::mat4x4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
		std::string m_strPoseClasses;                            // what classes we saw poses for this frame
		char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

		int m_iValidPoseCount = 0;

		glm::mat4x4 m_mat4HMDPose;
		glm::mat4x4 m_mat4eyePosLeft;
		glm::mat4x4 m_mat4eyePosRight;
		
		glm::mat4x4 m_mat4ProjectionCenter;
		glm::mat4x4 m_mat4ProjectionLeft;
		glm::mat4x4 m_mat4ProjectionRight;

		ApplicationSettings* appSettings;

		bool enableVR = false;
	};

	inline glm::mat4x4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);
}
