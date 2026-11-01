#include "vidra/gfx/renderer.hpp"
#include "vidra/core/log.hpp"
#include "vidra/tokens.hpp"
#include <X11/Xlib.h>
#include <cstdio>
#include <cstring>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vector>

namespace vidra::gfx {
namespace {

constexpr uint32_t kF = 2;
constexpr VkFormat kFmt = VK_FORMAT_B8G8R8A8_UNORM;

std::vector<char> slurp(const char* p) {
  FILE* f = std::fopen(p, "rb");
  if (!f) return {};
  std::vector<char> o;
  char b[4096];
  size_t n;
  while ((n = std::fread(b, 1, sizeof(b), f)) > 0) o.insert(o.end(), b, b + n);
  std::fclose(f);
  return o;
}
struct Globals {
  float screen[2]; float time; float hover;
  float tint[4]; float surface[4]; float light[4];
  float neighbors[8][4]; float gp[4]; float gp2[4]; float gp3[4];
};
struct QuadU { float box[4]; float color[4]; float uv[4]; float p0[4]; float p1[4]; };
struct BlurU { float dir[2]; float sigma; float pad; };

uint32_t memType(VkPhysicalDevice p, uint32_t bits, VkMemoryPropertyFlags want) {
  VkPhysicalDeviceMemoryProperties mp{};
  vkGetPhysicalDeviceMemoryProperties(p, &mp);
  for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
    if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) return i;
  }
  return 0;
}

struct Tex { VkImage img = VK_NULL_HANDLE; VkImageView view = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; };

class VkRenderer final : public Renderer {
 public:
  bool init(void* display, uint64_t window, int w, int h, std::string* err) override {
    dpy_ = static_cast<Display*>(display);
    win_ = Window(window);
    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ai.apiVersion = VK_API_VERSION_1_1;
    const char* ext[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &ai;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = ext;
    if (vkCreateInstance(&ici, nullptr, &inst_) != VK_SUCCESS) { if (err) *err = "instance"; return false; }
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst_, &n, nullptr);
    if (!n) { if (err) *err = "sem GPU"; return false; }
    std::vector<VkPhysicalDevice> ds(n);
    vkEnumeratePhysicalDevices(inst_, &n, ds.data());
    phys_ = ds[0];
    VkXlibSurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
    sci.dpy = dpy_; sci.window = win_;
    if (vkCreateXlibSurfaceKHR(inst_, &sci, nullptr, &surf_) != VK_SUCCESS) { if (err) *err = "surface"; return false; }
    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &n, nullptr);
    std::vector<VkQueueFamilyProperties> fs(n);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &n, fs.data());
    bool found = false;
    for (uint32_t i = 0; i < n; ++i) {
      VkBool32 pres = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(phys_, i, surf_, &pres);
      if ((fs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && pres) { qf_ = i; found = true; break; }
    }
    if (!found) { if (err) *err = "sem fila"; return false; }
    const float pr = 1.0f;
    VkDeviceQueueCreateInfo qc{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qc.queueFamilyIndex = qf_; qc.queueCount = 1; qc.pQueuePriorities = &pr;
    const char* de[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dc.queueCreateInfoCount = 1; dc.pQueueCreateInfos = &qc;
    dc.enabledExtensionCount = 1; dc.ppEnabledExtensionNames = de;
    if (vkCreateDevice(phys_, &dc, nullptr, &dev_) != VK_SUCCESS) { if (err) *err = "device"; return false; }
    vkGetDeviceQueue(dev_, qf_, 0, &queue_);
    VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pc.queueFamilyIndex = qf_; pc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(dev_, &pc, nullptr, &pool_);
    for (uint32_t i = 0; i < kF; ++i) {
      VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
      ca.commandPool = pool_; ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount = 1;
      vkAllocateCommandBuffers(dev_, &ca, &cmd_[i]);
      VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      vkCreateSemaphore(dev_, &si, nullptr, &si_[i]);
      vkCreateSemaphore(dev_, &si, nullptr, &sd_[i]);
      VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
      fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      vkCreateFence(dev_, &fi, nullptr, &fe_[i]);
    }
    if (!buildLayout(err)) return false;
    if (!buildPipelines(err)) return false;
    resize(w, h);
    ok_ = true;
    return true;
  }

  void shutdown() override {
    if (dev_) vkDeviceWaitIdle(dev_);
    destroyTargets();
    if (pipeScene_) vkDestroyPipeline(dev_, pipeScene_, nullptr);
    if (pipeBlit_) vkDestroyPipeline(dev_, pipeBlit_, nullptr);
    if (pipeBlur_) vkDestroyPipeline(dev_, pipeBlur_, nullptr);
    if (lay_) vkDestroyPipelineLayout(dev_, lay_, nullptr);
    for (auto p : {pass_}) if (p) vkDestroyRenderPass(dev_, p, nullptr);
    for (uint32_t i = 0; i < kF; ++i) {
      if (fe_[i]) vkDestroyFence(dev_, fe_[i], nullptr);
      if (si_[i]) vkDestroySemaphore(dev_, si_[i], nullptr);
      if (sd_[i]) vkDestroySemaphore(dev_, sd_[i], nullptr);
    }
    if (pool_) vkDestroyCommandPool(dev_, pool_, nullptr);
    if (dev_) vkDestroyDevice(dev_, nullptr);
    if (surf_) vkDestroySurfaceKHR(inst_, surf_, nullptr);
    if (inst_) vkDestroyInstance(inst_, nullptr);
  }

  void resize(int w, int h) override {
    if (!dev_ || w <= 0 || h <= 0) return;
    vkDeviceWaitIdle(dev_);
    destroyTargets();
    w_ = w; h_ = h;
    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = surf_; ci.minImageCount = 3; ci.imageFormat = kFmt;
    ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    ci.imageExtent = {uint32_t(w), uint32_t(h)}; ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = VK_PRESENT_MODE_FIFO_KHR; ci.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(dev_, &ci, nullptr, &swap_) != VK_SUCCESS) return;
    uint32_t n = 0;
    vkGetSwapchainImagesKHR(dev_, swap_, &n, nullptr);
    swapImgs_.resize(n); vkGetSwapchainImagesKHR(dev_, swap_, &n, swapImgs_.data());
    views_.resize(n); fbs_.resize(n);
    for (uint32_t i = 0; i < n; ++i) {
      VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      vi.image = swapImgs_[i]; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = kFmt;
      vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCreateImageView(dev_, &vi, nullptr, &views_[i]);
      VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      fi.renderPass = pass_; fi.attachmentCount = 1; fi.pAttachments = &views_[i];
      fi.width = uint32_t(w); fi.height = uint32_t(h); fi.layers = 1;
      vkCreateFramebuffer(dev_, &fi, nullptr, &fbs_[i]);
    }
    makeTex(scene_, w, h);
    makeTex(backdrop_, w, h);
    makeTex(blur_, std::max(1, w / 4), std::max(1, h / 4));
    makeFb(sceneFb_, scene_.view, w, h);
    makeFb(backdropFb_, backdrop_.view, w, h);
    makeFb(blurFb_, blur_.view, std::max(1, w / 4), std::max(1, h / 4));
  }

  void beginFrame(float t) override {
    if (!swap_) return;
    time_ = t;
    vkWaitForFences(dev_, 1, &fe_[f_], VK_TRUE, UINT64_MAX);
    if (vkAcquireNextImageKHR(dev_, swap_, UINT64_MAX, si_[f_], VK_NULL_HANDLE, &img_) != VK_SUCCESS) return;
    vkResetFences(dev_, 1, &fe_[f_]);
    vkResetCommandBuffer(cmd_[f_], 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd_[f_], &bi);
    beginPass(cmd_[f_], sceneFb_, w_, h_);
    tonal_ = 0;
    glass_ = 0;
    rec_ = true;
  }

  void push(const Quad& q) override {
    if (!rec_) return;
    QuadU u{};
    u.box[0] = q.box.x; u.box[1] = q.box.y; u.box[2] = q.box.w * 0.5f; u.box[3] = q.box.h * 0.5f;
    u.color[0] = q.color.r / 255.0f; u.color[1] = q.color.g / 255.0f;
    u.color[2] = q.color.b / 255.0f; u.color[3] = q.alpha;
    u.uv[0] = q.uv.x; u.uv[1] = q.uv.y; u.uv[2] = q.uv.w; u.uv[3] = q.uv.h;
    u.p0[0] = q.radius; u.p0[1] = q.exponent; u.p0[2] = q.notchDepth; u.p0[3] = tok::kNotchWidth;
    u.p1[0] = q.blur; u.p1[1] = q.thickness;
    const int m = int(q.mat);
    if (q.mat == Material::Glass) { glass_++; } else { tonal_++; }
    vkCmdBindPipeline(cmd_[f_], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeScene_);
    vkCmdPushConstants(cmd_[f_], lay_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(u), &u);
    vkCmdPushConstants(cmd_[f_], lay_, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(u), 4, &m);
    vkCmdDraw(cmd_[f_], 4, 1, 0, 0);
  }

  void endFrame() override {
    if (!rec_) return;
    rec_ = false;
    vkCmdEndRenderPass(cmd_[f_]);
    if (glass_ > 0) {
      transition(cmd_[f_], scene_.img, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
      transition(cmd_[f_], backdrop_.img, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      VkImageCopy cp{};
      cp.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      cp.dstSubresource = cp.srcSubresource;
      cp.extent = {uint32_t(w_), uint32_t(h_), 1};
      vkCmdCopyImage(cmd_[f_], scene_.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     backdrop_.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
      transition(cmd_[f_], scene_.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      transition(cmd_[f_], backdrop_.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      blurPass(backdrop_.view, blurFb_, std::max(1, w_ / 4), std::max(1, h_ / 4), true);
      blurPass(blur_.view, backdropFb_, w_, h_, false);
    }
    beginPass(cmd_[f_], fbs_[img_], w_, h_);
    vkCmdBindPipeline(cmd_[f_], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeBlit_);
    vkCmdDraw(cmd_[f_], 4, 1, 0, 0);
    vkCmdEndRenderPass(cmd_[f_]);
    vkEndCommandBuffer(cmd_[f_]);
    VkPipelineStageFlags w = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    s.waitSemaphoreCount = 1; s.pWaitSemaphores = &si_[f_]; s.pWaitDstStageMask = &w;
    s.commandBufferCount = 1; s.pCommandBuffers = &cmd_[f_];
    s.signalSemaphoreCount = 1; s.pSignalSemaphores = &sd_[f_];
    vkQueueSubmit(queue_, 1, &s, fe_[f_]);
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &sd_[f_];
    pi.swapchainCount = 1; pi.pSwapchains = &swap_; pi.pImageIndices = &img_;
    vkQueuePresentKHR(queue_, &pi);
    f_ = (f_ + 1) % kF;
  }

  BackendKind kind() const override { return BackendKind::Vulkan; }
  bool ok() const override { return ok_; }
  int width() const override { return w_; }
  int height() const override { return h_; }
  void setPalette(const float t[3], const float s[3], float hover) override {
    std::memcpy(tint_, t, sizeof(tint_)); std::memcpy(surf_, s, sizeof(surf_)); hover_ = hover;
  }
  void uploadAtlas(int, int, int, int, const uint8_t*) override {}

 private:
  bool buildLayout(std::string* err) {
    VkAttachmentDescription a{};
    a.format = kFmt; a.samples = VK_SAMPLE_COUNT_1_BIT;
    a.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; a.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference r{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sd{};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.colorAttachmentCount = 1; sd.pColorAttachments = &r;
    VkRenderPassCreateInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ri.attachmentCount = 1; ri.pAttachments = &a; ri.subpassCount = 1; ri.pSubpasses = &sd;
    if (vkCreateRenderPass(dev_, &ri, nullptr, &pass_) != VK_SUCCESS) { if (err) *err = "renderpass"; return false; }
    VkPushConstantRange pr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(QuadU) + 4};
    VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    li.pushConstantRangeCount = 1; li.pPushConstantRanges = &pr;
    return vkCreatePipelineLayout(dev_, &li, nullptr, &lay_) == VK_SUCCESS;
  }

  VkPipeline pipe(VkShaderModule a, VkShaderModule b, bool blend) {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = a; st[0].pName = "main";
    st[1] = st[0]; st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = b;
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1; vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState ba{};
    ba.colorWriteMask = 0xF;
    if (blend) {
      ba.blendEnable = VK_TRUE;
      ba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      ba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ba.colorBlendOp = VK_BLEND_OP_ADD;
      ba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ba.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1; cb.pAttachments = &ba;
    VkDynamicState dy[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ds.dynamicStateCount = 2; ds.pDynamicStates = dy;
    VkGraphicsPipelineCreateInfo g{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    g.stageCount = 2; g.pStages = st; g.pVertexInputState = &vi; g.pInputAssemblyState = &ia;
    g.pViewportState = &vp; g.pRasterizationState = &rs; g.pMultisampleState = &ms;
    g.pColorBlendState = &cb; g.pDynamicState = &ds; g.layout = lay_; g.renderPass = pass_;
    VkPipeline p = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(dev_, VK_NULL_HANDLE, 1, &g, nullptr, &p);
    return p;
  }

  bool buildPipelines(std::string* err) {
    auto mod = [&](const char* path) {
      auto c = slurp(path);
      if (c.empty()) { if (err) *err = std::string("shader ") + path; return VkShaderModule(VK_NULL_HANDLE); }
      VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
      ci.codeSize = c.size(); ci.pCode = reinterpret_cast<const uint32_t*>(c.data());
      VkShaderModule m = VK_NULL_HANDLE;
      vkCreateShaderModule(dev_, &ci, nullptr, &m);
      return m;
    };
    VkShaderModule vs = mod("scene.vert.spv"), fs = mod("scene.frag.spv");
    VkShaderModule bvs = mod("blur.vert.spv"), bfs = mod("blur.frag.spv");
    if (!vs || !fs || !bvs || !bfs) return false;
    pipeScene_ = pipe(vs, fs, true);
    pipeBlit_ = pipe(vs, fs, false);
    pipeBlur_ = pipe(bvs, bfs, false);
    for (auto m : {vs, fs, bvs, bfs}) vkDestroyShaderModule(dev_, m, nullptr);
    return pipeScene_ && pipeBlit_ && pipeBlur_;
  }

  void makeTex(Tex& t, int w, int h) {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D; ci.format = kFmt;
    ci.extent = {uint32_t(w), uint32_t(h), 1}; ci.mipLevels = 1; ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT; ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkCreateImage(dev_, &ci, nullptr, &t.img);
    VkMemoryRequirements mr{};
    vkGetImageMemoryRequirements(dev_, t.img, &mr);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = mr.size;
    ai.memoryTypeIndex = memType(phys_, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(dev_, &ai, nullptr, &t.mem);
    vkBindImageMemory(dev_, t.img, t.mem, 0);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = t.img; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = kFmt;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(dev_, &vi, nullptr, &t.view);
  }

  void makeFb(VkFramebuffer& fb, VkImageView v, int w, int h) {
    VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fi.renderPass = pass_; fi.attachmentCount = 1; fi.pAttachments = &v;
    fi.width = uint32_t(w); fi.height = uint32_t(h); fi.layers = 1;
    vkCreateFramebuffer(dev_, &fi, nullptr, &fb);
  }

  void transition(VkCommandBuffer c, VkImage img, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = from; b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
                      VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &b);
  }

  void beginPass(VkCommandBuffer c, VkFramebuffer fb, int w, int h) {
    VkClearValue cv{};
    cv.color = {{0, 0, 0, 0}};
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = pass_; rp.framebuffer = fb;
    rp.renderArea = {{0, 0}, {uint32_t(w), uint32_t(h)}};
    rp.clearValueCount = 1; rp.pClearValues = &cv;
    vkCmdBeginRenderPass(c, &rp, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vp{0, 0, float(w), float(h), 0, 1};
    VkRect2D sc{{0, 0}, {uint32_t(w), uint32_t(h)}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
  }

  void blurPass(VkImageView src, VkFramebuffer fb, int w, int h, bool horiz) {
    beginPass(cmd_[f_], fb, w, h);
    BlurU b{};
    b.dir[0] = horiz ? 1.0f / float(w) : 0.0f;
    b.dir[1] = horiz ? 0.0f : 1.0f / float(h);
    b.sigma = 6.0f;
    vkCmdBindPipeline(cmd_[f_], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeBlur_);
    vkCmdPushConstants(cmd_[f_], lay_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(b), &b);
    vkCmdDraw(cmd_[f_], 4, 1, 0, 0);
    vkCmdEndRenderPass(cmd_[f_]);
    (void)src;
  }

  void destroyTargets() {
    for (auto fb : fbs_) if (fb) vkDestroyFramebuffer(dev_, fb, nullptr);
    for (auto v : views_) if (v) vkDestroyImageView(dev_, v, nullptr);
    fbs_.clear(); views_.clear(); swapImgs_.clear();
    for (Tex* t : {&scene_, &backdrop_, &blur_}) {
      if (t->view) vkDestroyImageView(dev_, t->view, nullptr);
      if (t->img) vkDestroyImage(dev_, t->img, nullptr);
      if (t->mem) vkFreeMemory(dev_, t->mem, nullptr);
      *t = Tex{};
    }
    for (auto fb : {sceneFb_, backdropFb_, blurFb_}) if (fb) vkDestroyFramebuffer(dev_, fb, nullptr);
    sceneFb_ = backdropFb_ = blurFb_ = VK_NULL_HANDLE;
    if (swap_) { vkDestroySwapchainKHR(dev_, swap_, nullptr); swap_ = VK_NULL_HANDLE; }
  }

  Display* dpy_ = nullptr;
  Window win_ = 0;
  VkInstance inst_ = VK_NULL_HANDLE;
  VkPhysicalDevice phys_ = VK_NULL_HANDLE;
  VkDevice dev_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  uint32_t qf_ = 0;
  VkSurfaceKHR surf_ = VK_NULL_HANDLE;
  VkSwapchainKHR swap_ = VK_NULL_HANDLE;
  VkRenderPass pass_ = VK_NULL_HANDLE;
  VkPipelineLayout lay_ = VK_NULL_HANDLE;
  VkPipeline pipeScene_ = VK_NULL_HANDLE, pipeBlit_ = VK_NULL_HANDLE, pipeBlur_ = VK_NULL_HANDLE;
  VkCommandPool pool_ = VK_NULL_HANDLE;
  VkCommandBuffer cmd_[kF]{};
  VkSemaphore si_[kF]{}, sd_[kF]{};
  VkFence fe_[kF]{};
  std::vector<VkImage> swapImgs_;
  std::vector<VkImageView> views_;
  std::vector<VkFramebuffer> fbs_;
  Tex scene_, backdrop_, blur_;
  VkFramebuffer sceneFb_ = VK_NULL_HANDLE, backdropFb_ = VK_NULL_HANDLE, blurFb_ = VK_NULL_HANDLE;
  uint32_t f_ = 0, img_ = 0;
  int w_ = 0, h_ = 0, tonal_ = 0, glass_ = 0;
  float time_ = 0, hover_ = 0;
  float tint_[3]{0.42f, 0.31f, 0.64f}, surf_[3]{0.02f, 0.02f, 0.03f};
  bool ok_ = false, rec_ = false;
};

} // namespace

Renderer* createVkRenderer() { return new VkRenderer(); }

} // namespace vidra::gfx
