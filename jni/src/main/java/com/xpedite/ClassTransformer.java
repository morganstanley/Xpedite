///////////////////////////////////////////////////////////////////////////////
//
// Custom class transformer to transform Java classes inserting Xpedite probes
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite;
import com.xpedite.probes.AbstractProbe;
import com.xpedite.probes.AnchoredProbe;
import com.xpedite.probes.CallSite;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.NotFoundException;

import java.io.IOException;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.security.ProtectionDomain;

public class ClassTransformer implements ClassFileTransformer {
    private AbstractProbe[] probes;
    public ClassTransformer(AbstractProbe[] probes) {
        this.probes = probes;
    }

    @Override
    public byte[] transform(ClassLoader loader, String className, Class<?> classBeingRedefined,
                            ProtectionDomain protectionDomain, byte[] classfileBuffer) throws IllegalClassFormatException {

        byte[] bytecode = classfileBuffer;
        try {
            ClassPool cPool = ClassPool.getDefault();
            for (AbstractProbe probe: probes) {
                if (!probe.getClassName().equals(className)) {
                    continue;
                }
                CtClass ctClass = cPool.get(probe.getClassName().replace('/', '.'));
                CtMethod ctClassMethod = ctClass.getDeclaredMethod(probe.getMethodName());
                if (ctClassMethod == null) {
                    continue;
                }
                CallSite[] callSites = probe.getCallSites();
                if (callSites.length > 1) {
                    ctClassMethod.insertBefore(buildTrampoline(callSites[0].getId()));
                    ctClassMethod.insertAfter(buildTrampoline(callSites[1].getId()), true);
                } else {
                    ctClassMethod.insertAt(((AnchoredProbe) probe).getLineNo(), buildTrampoline(callSites[0].getId()));
                }
                bytecode = ctClass.toBytecode();
                ctClass.detach();
            }
        } catch (IOException | RuntimeException e) {
            throw new IllegalClassFormatException(e.getMessage());
        } catch (NotFoundException | CannotCompileException e) {
            e.printStackTrace();
        }
        return bytecode;
    }

    private String buildTrampoline(int id) {
        StringBuffer trampoline = new StringBuffer("com.xpedite.Xpedite.record(");
        trampoline.append(id);
        trampoline.append(");");
        return trampoline.toString();
    }
}
