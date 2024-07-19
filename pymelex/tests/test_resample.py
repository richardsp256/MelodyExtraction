import numpy as np

from pymelex import resample

def test_resample():
    # just going to see if this works
    rng = np.random.Generator(
	np.random.MT19937(np.random.SeedSequence(298226793))
    )
    white_noise = rng.uniform(low = -1., high = 1.,
			      size = 22050).astype(np.single)

    # Let's downsample our white noise
    rslt = resample(white_noise, 0.5)
    assert np.abs(white_noise.size*0.5 - rslt.size)<=1
