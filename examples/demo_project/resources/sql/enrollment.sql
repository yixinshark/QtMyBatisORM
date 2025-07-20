<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="Enrollment">
    <!-- 基础查询 -->
    <select id="findAll" resultType="QVariantMap">
        SELECT id, student_id, course_id, enrollment_date, grade, status, created_at, updated_at 
        FROM enrollments
        ORDER BY created_at DESC
    </select>

    <select id="findById" resultType="QVariantMap">
        SELECT id, student_id, course_id, enrollment_date, grade, status, created_at, updated_at 
        FROM enrollments 
        WHERE id = :arg1
    </select>

    <select id="findByStudentId" resultType="QVariantMap">
        SELECT id, student_id, course_id, enrollment_date, grade, status, created_at, updated_at 
        FROM enrollments 
        WHERE student_id = :arg1
        ORDER BY enrollment_date DESC
    </select>

    <select id="findByCourseId" resultType="QVariantMap">
        SELECT id, student_id, course_id, enrollment_date, grade, status, created_at, updated_at 
        FROM enrollments 
        WHERE course_id = :arg1
        ORDER BY enrollment_date DESC
    </select>

    <select id="count" resultType="int">
        SELECT COUNT(*) FROM enrollments
    </select>

    <!-- 检查选课状态 -->
    <select id="isStudentEnrolledInCourse" resultType="int">
        SELECT COUNT(*) FROM enrollments 
        WHERE student_id = :arg1 AND course_id = :arg2
    </select>

    <select id="findByStudentAndCourse" resultType="QVariantMap">
        SELECT id, student_id, course_id, enrollment_date, grade, status, created_at, updated_at 
        FROM enrollments 
        WHERE student_id = :arg1 AND course_id = :arg2
    </select>

    <!-- 统计查询 -->
    <select id="getAverageGrade" resultType="double">
        SELECT AVG(grade) FROM enrollments 
        WHERE grade IS NOT NULL AND status = 'COMPLETED'
    </select>

    <select id="getStudentAverageGrade" resultType="double">
        SELECT AVG(grade) FROM enrollments 
        WHERE student_id = :arg1 AND grade IS NOT NULL AND status = 'COMPLETED'
    </select>

    <!-- 存在性检查 -->
    <select id="existsEnrollment" resultType="int">
        SELECT COUNT(*) FROM enrollments 
        WHERE student_id = :arg1 AND course_id = :arg2
    </select>

    <!-- 统计查询 - 按学生获取平均成绩 -->
    <select id="getAverageGradeByStudent" resultType="QVariantMap">
        SELECT student_id, AVG(grade) as average_grade
        FROM enrollments 
        WHERE grade IS NOT NULL AND status = 'COMPLETED'
        GROUP BY student_id
    </select>

    <select id="getMostPopularCourses" resultType="QVariantMap">
        SELECT c.id as course_id, c.course_code, c.name, COUNT(e.id) as enrollment_count
        FROM courses c
        LEFT JOIN enrollments e ON c.id = e.course_id
        WHERE c.status = 'ACTIVE'
        GROUP BY c.id, c.course_code, c.name
        ORDER BY enrollment_count DESC
        LIMIT :arg1
    </select>

    <select id="countByStatus" resultType="QVariantMap">
        SELECT status, COUNT(*) as count
        FROM enrollments
        GROUP BY status
    </select>

    <select id="getGradeDistribution" resultType="QVariantMap">
        SELECT 
            CASE 
                WHEN grade >= 90 THEN 'A'
                WHEN grade >= 80 THEN 'B'
                WHEN grade >= 70 THEN 'C'
                WHEN grade >= 60 THEN 'D'
                ELSE 'F'
            END as grade_level,
            COUNT(*) as count
        FROM enrollments
        WHERE grade IS NOT NULL
        GROUP BY 
            CASE 
                WHEN grade >= 90 THEN 'A'
                WHEN grade >= 80 THEN 'B'
                WHEN grade >= 70 THEN 'C'
                WHEN grade >= 60 THEN 'D'
                ELSE 'F'
            END
        ORDER BY grade_level
    </select>

    <select id="getTopPerformingStudents" resultType="QVariantMap">
        SELECT e.student_id, s.student_number, s.name, AVG(e.grade) as gpa
        FROM enrollments e
        JOIN students s ON e.student_id = s.id
        WHERE e.grade IS NOT NULL AND e.status = 'COMPLETED'
        GROUP BY e.student_id, s.student_number, s.name
        HAVING AVG(e.grade) >= :arg1
        ORDER BY gpa DESC
        LIMIT :arg2
    </select>

    <!-- 复杂查询 -->
    <select id="findStudentEnrollmentsWithDetails" resultType="QVariantMap">
        SELECT e.*, c.course_code, c.name as course_name
        FROM enrollments e
        JOIN courses c ON e.course_id = c.id
        WHERE e.student_id = :arg1
        ORDER BY e.enrollment_date DESC
    </select>

    <select id="findCourseEnrollmentsWithDetails" resultType="QVariantMap">
        SELECT e.*, s.student_number, s.name as student_name
        FROM enrollments e
        JOIN students s ON e.student_id = s.id
        WHERE e.course_id = :arg1
        ORDER BY e.enrollment_date DESC
    </select>

    <!-- 插入操作 -->
    <insert id="insert">
        INSERT INTO enrollments (student_id, course_id, enrollment_date, grade, status)
        VALUES (:student_id, :course_id, :enrollment_date, :grade, :status)
    </insert>

    <!-- 更新操作 -->
    <update id="updateGrade">
        UPDATE enrollments 
        SET grade = :grade, status = :status, updated_at = CURRENT_TIMESTAMP
        WHERE student_id = :student_id AND course_id = :course_id
    </update>

    <update id="updateStatus">
        UPDATE enrollments 
        SET status = :status, updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    </update>

    <!-- 删除操作 -->
    <delete id="deleteById">
        DELETE FROM enrollments WHERE id = :arg1
    </delete>

    <delete id="deleteByStudentAndCourse">
        DELETE FROM enrollments 
        WHERE student_id = :arg1 AND course_id = :arg2
    </delete>
</mapper> 