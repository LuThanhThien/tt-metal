from loguru import logger
import os
import torch
import torchvision
from torch import nn
from transformers import AutoImageProcessor


def load_resnet50_model(model_location_generator):
    # TODO: Can generalize the version to an arg
    torch_resnet50 = None
    if model_location_generator is not None:
        model_version = "IMAGENET1K_V1.pt"
        model_path = model_location_generator(model_version, model_subdir="ResNet50")
        if os.path.exists(model_path):
            torch_resnet50 = torchvision.models.resnet50()
            torch_resnet50.load_state_dict(torch.load(model_path))
    if torch_resnet50 is None:
        torch_resnet50 = torchvision.models.resnet50(weights=torchvision.models.ResNet50_Weights.IMAGENET1K_V1)
    return torch_resnet50


class TorchResnet50(nn.Module):
    def __init__(self, model_name: str = "microsoft/resnet-50", model_location_generator=None, image_processor=None):
        super(TorchResnet50, self).__init__()
        self.model_name = model_name
        if image_processor:
            self.image_processor = image_processor
        self.image_processor = AutoImageProcessor.from_pretrained(model_name)
        self.model = load_resnet50_model(model_location_generator)

    def eval(self):
        self.model.eval()

    def preprocess(self, x):
        return self.image_processor(x, return_tensors="pt")

    def forward(self, x):
        x = self.model(x)
        return x
