all: test.comp.spv a.vert.spv a.frag.spv

test.comp.spv: test.comp
	glslc -o $@ $<

a.vert.spv: a.vert
	glslc -o $@ $<

a.frag.spv: a.frag
	glslc -o $@ $<

clean:
	rm *.spv