# HTTPserver


The Hypertext Transfer Protocol (HTTP) is an application protocol for distributed,
collaborative, and hypermedia information systems. HTTP is the
foundation of data communication for the World Wide Web. HTTP is built
on top of TCP layer.You can consult the RFC [1] and Wikipedia [2] for more
detailed information.
HTTP applications (applications that use HTTP to exchange hypertexts)
usually involve two entities, HTTP client and HTTP server. HTTP client is
the web browser for most cases while the HTTP server is one piece of program
running on server to provide web service.
In this course project, you are required to design and implement the HTTP
server and use your web browser to test your HTTP server.


You should:
- [x] 1. Implement the HTTP server using C/C++ (recommended), Java or Python.
- [x] 2. Support multiple threads.
- [x] 3. Support parse for different files. Such as HTML, CSS, JPG, PDF and
PPTX.
- [x] 4. Support gzip content-encoding/compression.
- [x] 5. Support chunked transfer encoding.
- [x] 6. Prepare several HTML files for testing. The content of the HTML files
should include at least your name, your student ID, one image and one
link to PDF file. The HTML files should use at least one CSS file.


how to run:


make


./server


open a brower to localhost:12345