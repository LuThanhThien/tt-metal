import ttnn
from torch import nn


class ModelConfig:
    def __init__(
        self,
        math_fidelity,
        weights_dtype,
        activations_dtype,
    ):
        self.math_fidelity = math_fidelity
        self.weights_dtype = weights_dtype
        self.activations_dtype = activations_dtype


class BaseModel(nn.Module):
    def __init__(
        self,
        device: str,
        parameters: dict,
        batch_size: int,
        model_config: ModelConfig,
        preprocessor=None,
        # metadata
        model_name: str = "base_model",
    ):
        super(BaseModel, self).__init__()
        self.device = device
        self.parameters = parameters
        self.batch_size = batch_size
        self.model_config = model_config
        self.preprocessor = preprocessor
        self.model_name = model_name

    def preprocess(self, x):
        return self.preprocessor(x)
