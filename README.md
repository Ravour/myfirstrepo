# myfirstrepo
上述思路只是直观上的想法，仍需要严谨的数学推导去证明其可行性以及明确优化目标。其实说不好VAE的产生到底是由理论推动的还是由直观思路推动的，其实是相辅相成的，总之VAE是一个从各个角度解释都很漂亮的模型。

我们的目标是根据x的某些特征z生成一个x。x是可观测的变量，而z是隐变量，所以我们需要反过来根据x去找出z的分布，也就是去求 $p(z \vert x)$ 。根据贝叶斯估计有

\[p(z \vert x) = \frac{p(x \vert z)p(z)}{p(x) } \\ p(x) = \int p(x \vert z)p(z)dz\]
显然在高维情况下这个积分没有解析形式，$p(x)$ 是求不出来的。而且如果能把 $p(x)$ 求出来，也就不需要什么隐变量了，分布直接就是最完美的模型了。有两种方法解决这个问题，一个是用MCMC去模拟估计出这个积分，另一个就是变分推断。

VAE就是采用了变分推断的思想去估计 $p(z \vert x)$ 的。假设存在一个可以用参数描述的分布 $q(z \vert x)$ ，使其与 $p(z \vert x)$ 非常相似，我们就可以用它来代替 $p(z \vert x)$ 这个分布。这种相似是用KL散度来衡量的，所以我们的目标变成了最小化 $KL(q(z \vert x) \parallel p(z \vert x))$ 。下面推导一下这个KL散度等于什么。

\[\begin{align*} KL(q(z \vert x) \parallel p(z \vert x)) & = -\sum q(z \vert x) \log \frac{p(z \vert x)}{q(z \vert x)}\\ & = -\sum q(z \vert x) \log \frac{\frac{p(x,z)}{p(x)}}{q(z \vert x)}\\ & = -\sum q(z \vert x) \left[\log \frac{p(x,z)}{q(z \vert x)} - \log p(x) \right]\\ & = -\sum q(z \vert x) \log \frac{p(x,z)}{q(z \vert x)} + \sum q(z \vert x) \log p(x)\\ & = -\sum q(z \vert x) \log \frac{p(x,z)}{q(z \vert x)} + \log p(x) \sum_z q(z \vert x) \\ & = -\sum q(z \vert x) \log \frac{p(x,z)}{q(z \vert x)} + \log p(x) \cdot 1 \\ \end{align*}\]
最终得到

\[\log p(x) = KL(q(z \vert x) \parallel p(z \vert x)) + \sum q(z \vert x) \log \frac{p(x,z)}{q(z \vert x)}\]
因为x是给定的观测值，所以这个等式的左边是一个常数。我们的目标是最小化右边第一项的KL散度，所以就变成了要最大化右边的第二项。我们称这项为变分下界，一般的变分推断的目标都是去最大化这个变分下界。通常情况下，这个变分下界都是很难去最大化的，而VAE通过巧妙的变化，把它变得易于理解。那么继续来看一下这个变分下界是什么。

\[\begin{align*} \sum q(z \vert x) \log \frac{p(x,z)}{q(z \vert x)} & = \sum q(z \vert x) \log \frac{p(x \vert z)p(z)}{q(z \vert x)}\\ & = \sum q(z \vert x) \left[ \log p(x \vert z) + \log \frac{p(z)}{q(z \vert x)}\right]\\ & = \sum q(z \vert x) \log p(x \vert z) + \sum q(z \vert x)\log \frac{p(z)}{q(z \vert x)}\\ & = E_{q(z \vert x)} \log p(x \vert z) - KL(q(z \vert x) \parallel p(z)) \end{align*}\]
最终我们的目标变成了最大化下面的式子（变分下界）。

\[E_{q(z \vert x)} \log p(x \vert z) - KL(q(z \vert x) \parallel p(z))\]
先看第一项，最大化变分下界需要最大化第一项，而最大化第一项其实就是最小化重建误差。为什么呢？设 $\hat x$ 是 $z$ 通过decoder后重建得到的新数据，因为decoder是一个神经网络，$z$ 和 $\hat x$ 有着对应关系，所以 $p(x \vert z)$ 相当于 $p(x \vert \hat x)$ 。又因为是关于 $q(z \vert x)$ 的期望，我们假设了z的分布为高斯分布，所以带入高斯分布解析式后最大化的东西就变成了 $e^{-\vert x - \hat x \vert^2}$ ，加上log就是最大化 $-\vert x - \hat x \vert^2$ ，也就是最小化 $\vert x - \hat x \vert^2$ ，所以第一项可以写成最小化重建误差 $\mathcal{L}(x, \hat x)$。

再看第二项，最大化变分下界需要最小化第二项。这个KL散度的意义也很明显，就是让我们假设的分布q近似于z的真实分布，与我们最初的目标相符合。由于我们假设z每个维度的分布都是独立的标准正态分布，所以也可以分开写成 $\sum_j{}KL(q_j(z \vert x) \parallel p(z))$ 。

最终我们得到的最小化目标，也就是损失函数为

\[\mathcal{L}(x, \hat x) + \sum_j{}KL(q_j(z \vert x) \parallel p(z))\]
我们知道 $x$ 是原数据， $z$ 是中间的隐变量，$\hat x$是重构数据，那么一个常规的想法就是用神经网络去拟合他们之间的映射。于是我们用encoder来拟合 $q(z \vert x)$ ，用decoder来拟合 $p(x \vert z)$，隐变量z正好作为中间层。如下图所示。
