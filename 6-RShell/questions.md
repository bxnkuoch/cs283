1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

_The remote client determines the completion of a command's output using specific markers (like EOF characters). To manage partial reads or ensure full message transmission, techniques like buffering incoming data, using predefined delimiters, or including length prefixes in the message can be employed._

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

_The networked shell protocol should frame the message with specific structures, such as fixed-length headers or delimiters, and include EOF characters to mark the end of a message. If this is not handled properly, the system may misinterpret commands, leading to incorrect processing or incomplete data handling._

3. Describe the general differences between stateful and stateless protocols.

_Stateful protocols retain information across multiple requests, allowing them to maintain context. On the other hand, stateless protocols do not store any session information, handling each request independently without remembering previous interactions._

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

_Even though it is "unreliable", We use it when we need low latency and speed, such as real-time communications or time-sensitive data transmission, where the speed of delivery is more important than ensuring all packets are received._

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

_The operating system provides socket interfaces, which allow applications to perform network communications using various protocols. This allows for efficient data exchange over a network._
