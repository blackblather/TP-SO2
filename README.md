# TP-SO2
Arkanoid game coded in <b><i>C</i></b>, using the <b><i>Windows API</i></b>.<br>
<hr>
<h2>General Idea:</h2>
The game is built on a client-server architecture, and supports multiple players/spectators.<br>
Players can be on the same machine or different machines on the local network.
<hr>
<h2>Technicalities:</h2>
The software uses the following <i>Windows API</i> mechanisms:
<ul>
  <li>Events</li>
  <li>Mutexes</li>
  <li>Semaphores</li>
  <li>Threads</li>
  <li>Registry</li>
  <li>NamedPipes</li>
</ul>
For support for both ANSI and UNICODE, the software also uses 	&lt;tchar.h&gt;
