import torch.nn as nn
from torchvision import models

class AlexNet(nn.Module):
    def __int__(self):
        super(AlexNet, self).__init__()
        self.model = models.alexnet(pretrained=True)
        self.model.eval()

